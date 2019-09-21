//
// Created by lwj on 2019/9/20.
//

#ifndef UDPTUN_TOOL_H
#define UDPTUN_TOOL_H
#include <string>

const uint32_t BUF_SIZE = 2048;

typedef struct {
  ///正常接触到的ip地址,点号隔开
  std::string ip;
  ///正常主机序端口号
  uint32_t port;
  ///值得注意的是转化为的64位,前32位是网络字节序的ip地址,而后32位是主机字节序的port,这点需要注意
  uint64_t to_UInt64() const;
  int32_t from_UInt64(uint64_t UInt64_ip_port);
} ip_port_t;

///将网络字节序的ip和port转化为uint64_t类型的数据,结果与将ip和port转为主机序后调用ip_port_t的to_UInt64结果相同
void ip_port_netorder2uint64(const uint32_t &ip_netorder, const uint32_t &port_netorder, uint64_t &UInt64_ip_port);

int set_non_blocking(const int &fd);

int new_listen_socket(const std::string &ip, const size_t &port, int &fd);

int new_connected_socket(const std::string &remote_ip, const size_t &remote_port, int &fd);

#endif //UDPTUN_TOOL_H
