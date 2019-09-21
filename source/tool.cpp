//
// Created by lwj on 2019/9/20.
//
#include <arpa/inet.h>
#include <glog/logging.h>
#include "tool.h"

uint64_t ip_port_t::to_UInt64() const {
    uint32_t ip_uint32 = inet_addr(ip.c_str());
    if (ip_uint32 == 0) {
        LOG(ERROR) << "invalid ip address ip:" << ip;
        return 0;
    }
    uint64_t ret = ip_uint32;
    ret <<= 32u;
    ret += port;
    return ret;
}

int32_t ip_port_t::from_UInt64(uint64_t UInt64_ip_port) {
    uint32_t ip_uint32 = UInt64_ip_port >> 32u;
    in_addr s_addr;
    s_addr.s_addr = ip_uint32;
    ip = std::string(inet_ntoa(s_addr));
    if (ip.empty()) {
        LOG(ERROR) << "invalid UInt64_ip_port " << UInt64_ip_port;
        return -1;
    }
    port = (UInt64_ip_port << 32u) >> 32u;
    return 0;
}

