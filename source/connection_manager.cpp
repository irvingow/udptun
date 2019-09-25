//
// Created by lwj on 2019/9/20.
//
#include "connection_manager.h"
#include <glog/logging.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "random_generator.h"

ConnectionManager::ConnectionManager(const int32_t &epoll_fd,
                                     const ip_port_t &remote_ip_port,
                                     const int32_t &listen_fd,
                                     const int32_t &connected_fd) :
    epoll_fd_(epoll_fd), server_ip_port_(remote_ip_port), listen_fd_(listen_fd), connected_fd_(connected_fd) {
    bzero(send_buf_, sizeof(send_buf_));
    send_buf_len_ = 0;
    bzero(recv_buf_, sizeof(recv_buf_));
    recv_buf_len_ = 0;
}

int32_t ConnectionManager::AddConnection(const ip_port_t &ip_port,
                                         const int32_t &listen_fd,
                                         const int32_t &connected_fd, const uint32_t &conn_id) {
    uint64_t uint64_ip_port = ip_port.to_UInt64();
    if (uint64_ip_port == 0) {
        LOG(ERROR) << "failed to encode ip:" << ip_port.ip << " port" << ip_port.port << " to uint64_t";
        return -1;
    }
    if (Exist(uint64_ip_port)) {
        LOG(ERROR) << "ip:" << ip_port.ip << " port:" << ip_port.port << " already exists";
        return -1;
    }
    std::shared_ptr<Connection> sp_conn(new Connection(ip_port, listen_fd));
    uint64_ip_port2conn_hashmap_[uint64_ip_port] = sp_conn;
    auto ret = sp_conn->AddConnectedFdAndConnId(connected_fd, conn_id);
    if (ret < 0) {
        LOG(ERROR) << "failed to add connected fd and conn_id to Connection ip:" << ip_port.ip
                   << " port:" << ip_port.port;
        return -1;
    }
    LOG(INFO) << "Add new connection ip:" << ip_port.ip << " port:" << ip_port.port<< " uint64_ip_port:"<<uint64_ip_port;
    uint64_ip_port2conn_ids_hashmap_[uint64_ip_port].insert(conn_id);
    conn_id2uin64_ip_port_hashmap_[conn_id] = uint64_ip_port;
    connected_fd2uint64_ip_port_hashmap_[connected_fd] = uint64_ip_port;
    return 0;
}

int32_t ConnectionManager::ConnectionAddConnectedFd(const ip_port_t &ip_port,
                                                    const int32_t &connected_fd,
                                                    const uint32_t &conn_id) {
    uint64_t uint64_ip_port = ip_port.to_UInt64();
    if (uint64_ip_port == 0) {
        LOG(ERROR) << "failed to encode ip:" << ip_port.ip << " port" << ip_port.port << " to uint64_t";
        return -1;
    }
    if (!Exist(uint64_ip_port)) {
        LOG(ERROR) << "ip:" << ip_port.ip << " port:" << ip_port.port << " doesn't exist";
        return -1;
    }
    std::shared_ptr<Connection> sp_conn = uint64_ip_port2conn_hashmap_[uint64_ip_port];
    auto ret = sp_conn->AddConnectedFdAndConnId(connected_fd, conn_id);
    if (ret < 0) {
        LOG(ERROR) << "failed to add connected fd and conn_id to Connection ip:" << ip_port.ip
                   << " port:" << ip_port.port;
        return -1;
    }
    uint64_ip_port2conn_ids_hashmap_[uint64_ip_port].insert(conn_id);
    conn_id2uin64_ip_port_hashmap_[conn_id] = uint64_ip_port;
    connected_fd2uint64_ip_port_hashmap_[connected_fd] = uint64_ip_port;
}

bool ConnectionManager::Exist(const uint64_t &uint64_ip_port) {
    if (uint64_ip_port2conn_hashmap_.size() != uint64_ip_port2conn_ids_hashmap_.size()) {
        LOG(ERROR)
            << "this should not happen, uint64_ip_port2conn_ids_hashmap_ size:"
            << uint64_ip_port2conn_ids_hashmap_.size() <<
            " is not equal to uint64_ip_port2conn_hashmap_ size:" << uint64_ip_port2conn_hashmap_.size();
    }
    return uint64_ip_port2conn_hashmap_.count(uint64_ip_port);
}

///udptun server调用,接收到来自udptun_client的数据
void ConnectionManager::GotDataWithConnIdFromClient() {
    sockaddr_in addr;
    socklen_t slen = sizeof(addr);
    bzero(send_buf_, sizeof(send_buf_));
    auto ret = recvfrom(listen_fd_, send_buf_, BUF_SIZE, 0, (sockaddr *) &addr, &slen);
    if (ret > 0) {
        send_buf_len_ = ret;
        uint64_t uint64_ip_port = 0;
        ip_port_netorder2uint64(addr.sin_addr.s_addr, addr.sin_port, uint64_ip_port);
        ip_port_t client_ip_port;
        client_ip_port.from_UInt64(uint64_ip_port);
        uint32_t conn_id = 0;
        ret = GetConnIdFromData(conn_id, send_buf_, send_buf_len_);
        if (ret < 0) {
            LOG(ERROR) << "failed to get conn_id from data";
            return;
        }
        int32_t new_connected_fd = -1;
        if (!Exist(uint64_ip_port)) {
            ///如果此时connection_manager里没有这个ip_port对应的连接
            ret = new_connected_socket(server_ip_port_.ip, server_ip_port_.port, new_connected_fd);
            if (ret < 0) {
                LOG(ERROR) << "failed to create new_connected_socket";
                return;
            }
            ret = AddEvent2Epoll(epoll_fd_, new_connected_fd, EPOLLIN);
            if (ret < 0) {
                LOG(ERROR) << "failed to add new_connected_fd to epoll";
                close(new_connected_fd);
                return;
            }
            ret = AddConnection(client_ip_port, listen_fd_, new_connected_fd, conn_id);
            if (ret < 0) {
                LOG(ERROR) << "failed to add connection ip:" << client_ip_port.ip << " port:" << client_ip_port.port
                           << "to connection_manager";
                close(new_connected_fd);
                return;
            }
            LOG(INFO) << "add new connection ip:" << client_ip_port.ip << " port:" << client_ip_port.port
                      << " to connection_manager";
        }
        if (!uint64_ip_port2conn_ids_hashmap_[uint64_ip_port].count(conn_id)) {
            ///说明连接已经存在,但是没有对应的conn_id不存在
            ret = new_connected_socket(server_ip_port_.ip, server_ip_port_.port, new_connected_fd);
            if (ret < 0) {
                LOG(ERROR) << "failed to create new_connected_socket";
                return;
            }
            ret = AddEvent2Epoll(epoll_fd_, new_connected_fd, EPOLLIN);
            if (ret < 0) {
                LOG(ERROR) << "failed to add new_connected_fd to epoll";
                close(new_connected_fd);
                return;
            }
            ret = ConnectionAddConnectedFd(client_ip_port, new_connected_fd, conn_id);
            if (ret < 0) {
                LOG(ERROR) << "failed to add connected fd and conn_id to Connection ip:" << client_ip_port.ip
                           << " port:" << client_ip_port.port;
                close(new_connected_fd);
                return;
            }
        }
        connected_fd2conn_id_hashmap_[new_connected_fd] = conn_id;
        auto sp_conn = uint64_ip_port2conn_hashmap_[uint64_ip_port];
        memcpy(sp_conn->send_buf_, send_buf_, send_buf_len_);
        sp_conn->send_buf_len_ = send_buf_len_;
        sp_conn->GotDataFromClient(conn_id);
    } else if (ret < 0) {
        LOG(ERROR) << "failed to recv data from udptun client error:" << strerror(errno);
    }
}

///udptun client,接收到来自client的数据
void ConnectionManager::GotDataWithoutConnIdFromClient() {
    sockaddr_in addr;
    socklen_t slen = sizeof(addr);
    bzero(send_buf_, sizeof(send_buf_));
    auto ret = recvfrom(listen_fd_, send_buf_, BUF_SIZE, 0, (sockaddr *) &addr, &slen);
    if (ret > 0) {
        send_buf_len_ = ret;
        uint64_t uint64_ip_port = 0;
        ip_port_netorder2uint64(addr.sin_addr.s_addr, addr.sin_port, uint64_ip_port);
        ip_port_t client_ip_port;
        client_ip_port.from_UInt64(uint64_ip_port);
        uint32_t conn_id = 0;
        for (auto iter = conn_id2uin64_ip_port_hashmap_.begin(); iter != conn_id2uin64_ip_port_hashmap_.end(); ++iter) {
            if (iter->second == uint64_ip_port) {
                conn_id = iter->first;
            }
        }
        if (conn_id == 0) {
            ///说明没有对应的连接存在
            auto random_num_gen = RandomNumberGenerator::GetInstance();
            uint32_t unique_conn_id = 0;
            ret = random_num_gen->GetRandomNumberNonZero(unique_conn_id);
            if (uint64_ip_port2conn_ids_hashmap_.count(uint64_ip_port)) {
                while (unique_conn_id == 0 || uint64_ip_port2conn_ids_hashmap_[uint64_ip_port].count(unique_conn_id)) {
                    if (ret != 0)
                        break;
                    ret = random_num_gen->GetRandomNumberNonZero(unique_conn_id);
                }
            }
            else{
                while(unique_conn_id == 0){
                    if(ret != 0)
                        break;
                    ret = random_num_gen->GetRandomNumberNonZero(unique_conn_id);
                }
            }
            if (ret != 0) {
                LOG(ERROR) << "failed to get random_number";
                return;
            }
            ret = AddConnection(client_ip_port, listen_fd_, connected_fd_, unique_conn_id);
            if (ret < 0) {
                LOG(ERROR) << "failed to add connection ip:" << client_ip_port.ip << " port:" << client_ip_port.port
                           << "to connection_manager";
                return;
            }
            conn_id = unique_conn_id;
        }
        ret = PutConnIdIntoData(conn_id, send_buf_, send_buf_len_);
        if (ret < 0) {
            LOG(ERROR) << "failed to put conn_id into data";
            return;
        }
        auto sp_conn = uint64_ip_port2conn_hashmap_[uint64_ip_port];
        memcpy(sp_conn->send_buf_, send_buf_, send_buf_len_);
        sp_conn->send_buf_len_ = send_buf_len_;
        sp_conn->GotDataFromClient(conn_id);
    } else if (ret < 0) {
        LOG(ERROR) << "failed to recv data from udptun server error:" << strerror(errno);
    }
}

///udptun client调用,接收到来自udptun_server的数据
void ConnectionManager::GotDataWithConnIdFromServer() {
    sockaddr_in addr;
    socklen_t slen = sizeof(addr);
    bzero(recv_buf_, sizeof(recv_buf_));
    auto ret = recvfrom(connected_fd_, recv_buf_, BUF_SIZE, 0, (sockaddr *) &addr, &slen);
    if (ret > 0) {
        recv_buf_len_ = ret;
        uint32_t conn_id = 0;
        ret = GetConnIdFromData(conn_id, recv_buf_, recv_buf_len_);
        if (ret < 0) {
            LOG(ERROR) << "failed to get conn_id from data";
            return;
        }
        if (!conn_id2uin64_ip_port_hashmap_.count(conn_id) ) {
            LOG(ERROR)
                << "this should never happen, conn_id does not exist";
            return;
        }
        auto sp_conn = uint64_ip_port2conn_hashmap_[conn_id2uin64_ip_port_hashmap_[conn_id]];
        memcpy(sp_conn->recv_buf_, recv_buf_, recv_buf_len_);
        sp_conn->recv_buf_len_ = recv_buf_len_;
        sp_conn->GotDataFromServer();
    } else if (ret < 0) {
        LOG(ERROR) << "failed to recv data from udptun server error:" << strerror(errno);
    }
}

///udptun server,接收到来自server的数据
void ConnectionManager::GotDataWithoutConnIdFromServer(const int32_t &connected_fd) {
    bzero(recv_buf_, sizeof(recv_buf_));
    auto ret = recv(connected_fd, recv_buf_, BUF_SIZE, 0);
    if (ret > 0) {
        recv_buf_len_ = ret;
        if (!connected_fd2uint64_ip_port_hashmap_.count(connected_fd)
            || !connected_fd2conn_id_hashmap_.count(connected_fd)) {
            LOG(ERROR) << "failed to find correspond connected_fd:" << connected_fd;
            return;
        }
        uint64_t uint64_ip_port = connected_fd2uint64_ip_port_hashmap_[connected_fd];
        ip_port_t client_ip_port;
        client_ip_port.from_UInt64(uint64_ip_port);
        uint32_t conn_id = connected_fd2conn_id_hashmap_[connected_fd];
        ret = PutConnIdIntoData(conn_id, recv_buf_, recv_buf_len_);
        if (ret < 0) {
            LOG(ERROR) << "failed to put conn_id into data";
            return;
        }
        auto sp_conn = uint64_ip_port2conn_hashmap_[uint64_ip_port];
        memcpy(sp_conn->recv_buf_, recv_buf_, recv_buf_len_);
        sp_conn->recv_buf_len_ = recv_buf_len_;
        uint64_ip_port2conn_hashmap_[uint64_ip_port]->GotDataFromServer();
    } else if (ret < 0) {
        LOG(ERROR) << "failed to recv data from udptun server error:" << strerror(errno);
    }
}

























