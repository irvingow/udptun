//
// Created by lwj on 2019/9/20.
//
#include "connection_manager.h"
#include <glog/logging.h>

int32_t ConnectionManager::AddConnection(const ip_port_t &ip_port) {
    uint64_t UInt64_ip_port = ip_port.to_UInt64();
    if(UInt64_ip_port == 0){
        LOG(ERROR) << "invalid ip_port ip:"<<ip_port.ip<<" port:"<<ip_port.port;
        return -1;
    }
    std::shared_ptr<Connection> sp_conn(new Connection(ip_port));
    UInt64_ip_port_Conn_map_[UInt64_ip_port] = sp_conn;
    return 0;
}

int32_t ConnectionManager::RemoveConnection(const ip_port_t &ip_port) {
    uint64_t UInt64_ip_port = ip_port.to_UInt64();
    if(UInt64_ip_port == 0){
        LOG(ERROR) << "invalid ip_port ip:"<<ip_port.ip<<" port:"<<ip_port.port;
        return -1;
    }
    auto iter = UInt64_ip_port_Conn_map_.find(UInt64_ip_port);
    if(iter == UInt64_ip_port_Conn_map_.end()){
        LOG(WARNING)<<"ip_port ip:"<<ip_port.ip<<" port:"<<ip_port.port<<" doesn't exist";
        return 0;
    }
    UInt64_ip_port_Conn_map_.erase(iter);
    return 0;
}

bool ConnectionManager::Exist(const ip_port_t &ip_port) const {
    uint64_t UInt64_ip_port = ip_port.to_UInt64();
    if(UInt64_ip_port == 0){
        LOG(ERROR) << "invalid ip_port ip:"<<ip_port.ip<<" port:"<<ip_port.port;
        return false;
    }
    auto iter = UInt64_ip_port_Conn_map_.find(UInt64_ip_port);
    return iter != UInt64_ip_port_Conn_map_.end();
}
