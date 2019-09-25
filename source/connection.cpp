//
// Created by lwj on 2019/9/20.
//
#include "connection.h"
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glog/logging.h>

Connection::Connection(const ip_port_t &ip_port, const int32_t &local_listen_fd)
    : client_ip_port_(ip_port), listen_fd_(local_listen_fd) {
    bzero(send_buf_, sizeof(send_buf_));
    send_buf_len_ = 0;
    bzero(recv_buf_, sizeof(recv_buf_));
    recv_buf_len_ = 0;
}

int32_t Connection::AddConnectedFdAndConnId(const int32_t &connected_fd, const uint32_t &conn_id) {
    if (connected_fd2conn_id_hashmap_.count(connected_fd)) {
        LOG(ERROR) << "connected fd:" << connected_fd << " already exist and its conn_id is:" << conn_id;
        return -1;
    }
    conn_id2connected_fd_hashmap_[conn_id] = connected_fd;
    connected_fd2conn_id_hashmap_[connected_fd] = conn_id;
    uint64_t uint64_ip_port = client_ip_port_.to_UInt64();
    if (uint64_ip_port == 0) {
        LOG(ERROR) << "failed to encode ip:" << client_ip_port_.ip << " port" << client_ip_port_.port << " to uint64_t";
        return -1;
    }
    LOG(INFO) << "connection ip:" << client_ip_port_.ip << " port:" << client_ip_port_.port << " add connected fd:"
              << connected_fd << " conn_id:" << conn_id;
    return 0;
}

bool Connection::Exist(const uint32_t &conn_id) const {
    return conn_id2connected_fd_hashmap_.count(conn_id);
}

void Connection::GotDataFromServer() {
    sockaddr_in remote_addr;
    remote_addr.sin_addr.s_addr = inet_addr(client_ip_port_.ip.c_str());
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(client_ip_port_.port);
    auto ret = sendto(listen_fd_, recv_buf_,
                      recv_buf_len_, 0,
                      (sockaddr *) &remote_addr,
                      sizeof(remote_addr));
    if (ret < 0) {
        LOG(ERROR) << "send data to local_addr ip:" << client_ip_port_.ip << " port:" << client_ip_port_.port
                   << " error:"
                   << strerror(errno);
    }
}

void Connection::GotDataFromClient(const uint32_t &conn_id) {
    if (connected_fd2conn_id_hashmap_.size() != conn_id2connected_fd_hashmap_.size()) {
        LOG(ERROR)
            << "this should never happen, connected_fd2conn_id_hashmap_ size is not equal to conn_id2connected_fd_hashmap_ size";
        return;
    }
    if(conn_id == 0){
        LOG(ERROR)<<"conn_id can not be zero!";
        return;
    }
    if (!conn_id2connected_fd_hashmap_.count(conn_id)) {
        LOG(ERROR) << "conn_id:" << conn_id << " doesn't exist";
        return;
    }
    int32_t server_connected_fd = conn_id2connected_fd_hashmap_[conn_id];
    int send_ret =
        send(server_connected_fd, send_buf_, send_buf_len_, 0);
    if (send_ret < 0) {
        LOG(ERROR) << "send data to server failed error:"
                   << strerror(errno);
    }
}

