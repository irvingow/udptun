//
// Created by lwj on 2019/9/20.
//
#include "connection_manager.h"
#include <glog/logging.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "random_generator.h"

int32_t ConnectionManager::AddConnection(const ip_port_t &ip_port) {
    uint64_t UInt64_ip_port = ip_port.to_UInt64();
    if (UInt64_ip_port == 0) {
        LOG(ERROR) << "invalid ip_port ip:" << ip_port.ip << " port:" << ip_port.port;
        return -1;
    }
    /// 这里必须判断ip_port对应的连接是否已经存在,如果已经有对应的连接存在,直接覆盖之前的connection
    /// 会导致内存泄露,因为random_number不一致,所以前面一个连接的conn_id肯定和后面的不一样,所以
    /// 前一个连接的conn_id对应的智能指针没有析构,进而导致内存泄露
    auto uint64_iter = UInt64_ip_port_Conn_map_.find(UInt64_ip_port);
    if (uint64_iter != UInt64_ip_port_Conn_map_.end()) {
        LOG(WARNING) << "ip:" << ip_port.ip << " port:" << ip_port.port << " connection already exist!";
        return -1;
    }
    auto random_num_gen = RandomNumberGenerator::GetInstance();
    uint32_t unique_conn_id;
    int32_t ret = random_num_gen->GetRandomNumberNonZero(unique_conn_id);
    auto iter = unique_conn_id_map_.find(unique_conn_id);
    while (iter != unique_conn_id_map_.end() && ret == 0) {
        ret = random_num_gen->GetRandomNumberNonZero(unique_conn_id);
    }
    if (ret != 0) {
        LOG(ERROR) << "failed to get random_number";
        return -1;
    }
    std::shared_ptr<Connection> sp_conn(new Connection(ip_port, unique_conn_id));
    UInt64_ip_port_Conn_map_[UInt64_ip_port] = sp_conn;
    unique_conn_id_map_[unique_conn_id] = sp_conn;
    if (UInt64_ip_port_Conn_map_.size() != unique_conn_id_map_.size()) {
        LOG(ERROR) << "this should not happen, UInt64_ip_port_Conn_map size is not equal to unique_conn_id_map size";
        return -1;
    }
    return 0;
}

int32_t ConnectionManager::AddConnection(const uint64_t &uint64_ip_port) {
    ip_port_t ip_port;
    ip_port.from_UInt64(uint64_ip_port);
    auto uint64_iter = UInt64_ip_port_Conn_map_.find(uint64_ip_port);
    /// 这里必须判断ip_port对应的连接是否已经存在,如果已经有对应的连接存在,直接覆盖之前的connection
    /// 会导致内存泄露,因为random_number不一致,所以前面一个连接的conn_id肯定和后面的不一样,所以
    /// 前一个连接的conn_id对应的智能指针没有析构,进而导致内存泄露
    if (uint64_iter != UInt64_ip_port_Conn_map_.end()) {
        LOG(WARNING) << "ip:" << ip_port.ip << " port:" << ip_port.port << " connection already exist!";
        return -1;
    }
    auto random_num_gen = RandomNumberGenerator::GetInstance();
    uint32_t unique_conn_id;
    int32_t ret = random_num_gen->GetRandomNumberNonZero(unique_conn_id);
    auto iter = unique_conn_id_map_.find(unique_conn_id);
    while (iter != unique_conn_id_map_.end() && ret == 0) {
        ret = random_num_gen->GetRandomNumberNonZero(unique_conn_id);
    }
    if (ret != 0) {
        LOG(ERROR) << "failed to get random_number";
        return -1;
    }
    std::shared_ptr<Connection> sp_conn(new Connection(ip_port, unique_conn_id));
    UInt64_ip_port_Conn_map_[uint64_ip_port] = sp_conn;
    unique_conn_id_map_[unique_conn_id] = sp_conn;
    if (UInt64_ip_port_Conn_map_.size() != unique_conn_id_map_.size()) {
        LOG(ERROR) << "this should not happen, UInt64_ip_port_Conn_map size is not equal to unique_conn_id_map size";
        return -1;
    }
    return 0;
}

int32_t ConnectionManager::RemoveConnection(const ip_port_t &ip_port) {
    uint64_t UInt64_ip_port = ip_port.to_UInt64();
    if (UInt64_ip_port == 0) {
        LOG(ERROR) << "invalid ip_port ip:" << ip_port.ip << " port:" << ip_port.port;
        return -1;
    }
    auto iter = UInt64_ip_port_Conn_map_.find(UInt64_ip_port);
    if (iter == UInt64_ip_port_Conn_map_.end()) {
        LOG(WARNING) << "ip_port ip:" << ip_port.ip << " port:" << ip_port.port << " doesn't exist";
    } else {
        auto unique_conn_id = iter->second->unique_connection_id();
        UInt64_ip_port_Conn_map_.erase(iter);
        auto iter2 = unique_conn_id_map_.find(unique_conn_id);
        if (iter2 == unique_conn_id_map_.end()) {
            LOG(WARNING) << "this should not happen when ip_port exists but unique_connection_id:" << unique_conn_id
                         << " doesn't exist";
        } else {
            unique_conn_id_map_.erase(iter2);
        }
    }
    return 0;
}

bool ConnectionManager::Exist(const ip_port_t &ip_port) const {
    uint64_t uint64_ip_port = ip_port.to_UInt64();
    if (uint64_ip_port == 0) {
        LOG(ERROR) << "invalid ip_port ip:" << ip_port.ip << " port:" << ip_port.port;
        return false;
    }
    auto iter = UInt64_ip_port_Conn_map_.find(uint64_ip_port);
    return iter != UInt64_ip_port_Conn_map_.end();
}

bool ConnectionManager::Exist(const uint64_t &uint64_ip_port) const {
    auto iter = UInt64_ip_port_Conn_map_.find(uint64_ip_port);
    return iter != UInt64_ip_port_Conn_map_.end();
}

void ConnectionManager::SendMesgToLocal() {
    if (buf_len_ == 0) {
        LOG(WARNING) << "buf_len_ is zero!";
        return;
    }
    ip_port_t ip_port;
    auto ret = GetIpPortFromData(ip_port);
    if (ret != 0) {
        LOG(ERROR) << "failed to get ip_port from data";
        return;
    }
    ///test code because of GetIpPortFromData has not been implemented
    //TODO
    ip_port = UInt64_ip_port_Conn_map_.begin()->second->connection_ip_port();
    ///test code end
    sockaddr_in remote_addr;
    remote_addr.sin_addr.s_addr = inet_addr(ip_port.ip.c_str());
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htonl(ip_port.port);

    ret = sendto(fd_, buf_,
                 buf_len_, 0,
                 (sockaddr *) &remote_addr,
                 sizeof(remote_addr));
    if (ret < 0) {
        LOG(ERROR) << "send data to local_addr ip:" << ip_port.ip << " port:" << ip_port.port << " error:"
                   << strerror(errno);
    }
}

void ConnectionManager::SendMesgToRemote(const uint64_t &uint64_ip_port) {
    if (buf_len_ == 0) {
        LOG(WARNING) << "buf_len_ is zero!";
        return;
    }
    auto ret = PutConnIdToData(uint64_ip_port);
    if (ret != 0) {
        LOG(ERROR) << "failed to put uint64_ip_port to data";
        return;
    }
    int send_ret =
        send(fd_, buf_, buf_len_, 0);
    if (send_ret < 0) {
        auto ip_port = UInt64_ip_port_Conn_map_.begin()->second->connection_ip_port();
        ///理论上remote_connection_manager的UInt64_ip_port_Conn_map_中有且只有一个连接,那就是连接远端的连接
        LOG(ERROR) << "send data to remote failed ip:" << ip_port.ip << " port:" << ip_port.port << " error:"
                   << strerror(errno);
        return;
    }
}

int32_t ConnectionManager::GetIpPortFromData(ip_port_t &ip_port) {
    if(buf_len_ < 4){
        LOG(ERROR)<<" error udp data package, package length should not less than 4 bytes";
        return -1;
    }
    uint32_t netorder_conn_id = 0;
    memcpy(&netorder_conn_id, buf_ + buf_len_ - 4, 4);
    uint32_t unique_conn_id = ntohl(netorder_conn_id);
    buf_len_ -= sizeof(netorder_conn_id);
    auto iter = unique_conn_id_map_.find(unique_conn_id);
    if(iter == unique_conn_id_map_.end()){
        LOG(ERROR)<<"failed to find correspond connection";
        return -1;
    }
    ip_port = iter->second->connection_ip_port();
    return 0;
}

int32_t ConnectionManager::PutConnIdToData(const uint64_t &uint64_ip_port) {
    if(buf_len_ > BUF_SIZE){
        LOG(ERROR)<<"too big udp data package!!";
        return -1;
    }
    auto iter = UInt64_ip_port_Conn_map_.find(uint64_ip_port);
    if (iter == UInt64_ip_port_Conn_map_.end()) {
        LOG(ERROR) << "failed to find connection whose uint64_ip_port is " << uint64_ip_port;
        return -1;
    }
    uint32_t unique_conn_id = iter->second->unique_connection_id();
    uint32_t netorder_conn_id = htonl(unique_conn_id);
    memcpy(buf_+buf_len_, &netorder_conn_id, sizeof(netorder_conn_id));
    buf_len_ += sizeof(netorder_conn_id);
    return 0;
}



















