//
// Created by lwj on 2019/9/20.
//
#include <arpa/inet.h>
#include <glog/logging.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
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

void ip_port_netorder2uint64(const uint32_t& ip_netorder, const uint32_t& port_netorder, uint64_t& UInt64_ip_port){
    uint32_t port_hostorder = ntohl(port_netorder);
    UInt64_ip_port = ip_netorder;
    UInt64_ip_port <<= 32u;
    UInt64_ip_port += port_hostorder;
}

int set_non_blocking(const int &fd) {
    int opts = -1;
    opts = fcntl(fd, F_GETFL);

    if (opts < 0) {
        LOG(ERROR) << "get socket status failed, fd:" << fd
                   << " error:" << strerror(errno);
        return -1;
    }
    opts = opts | O_NONBLOCK;
    if (fcntl(fd, F_SETFL, opts) < 0) {
        LOG(ERROR) << "set socket non_blocking failed, fd:" << fd
                   << " error:" << strerror(errno);
        return -1;
    }
}


int new_listen_socket(const std::string &ip, const size_t &port, int& fd) {
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == -1) {
        LOG(ERROR) << "create new socket failed" << strerror(errno);
        return -1;
    }
    struct sockaddr_in local_addr = {0};
    socklen_t slen = sizeof(local_addr);
    local_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *) &local_addr, slen) == -1) {
        LOG(ERROR) << "socket bind error port:" << port
                   << " error:" << strerror(errno);
        return -1;
    }
    set_non_blocking(fd);
    LOG(INFO) << "local socket listen_fd:" << fd;
}

int new_connected_socket( const std::string &remote_ip,
                          const size_t &remote_port,int &fd) {
    LOG(INFO) << "remote ip:" << remote_ip << " port:" << remote_port;
    struct sockaddr_in remote_addr_in = {0};
    socklen_t slen = sizeof(remote_addr_in);
    remote_addr_in.sin_addr.s_addr = inet_addr(remote_ip.c_str());
    remote_addr_in.sin_family = AF_INET;
    remote_addr_in.sin_port = htons(remote_port);

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        LOG(ERROR) << "create new socket failed" << strerror(errno);
        return -1;
    }
    int ret = connect(fd, (struct sockaddr *) &remote_addr_in, slen);
    if (ret < 0) {
        ///实际上connect基本不会失败,因为内核只做一些基本的检查,比如参数是否合法,ip是否有效等等,
        ///之后内核就创建相应数据结构,并且返回,并没有真的去连接那个ip和port
        LOG(ERROR) << "failed to establish connection to remote, error:"
                   << strerror(errno);
        close(fd);
        return -1;
    }
    set_non_blocking(fd);
    LOG(INFO) << "create new remote connection udp_fd:" << fd;
    return 0;
}
