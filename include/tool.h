//
// Created by lwj on 2019/9/20.
//

#ifndef UDPTUN_TOOL_H
#define UDPTUN_TOOL_H
#include <string>

typedef struct  {
  std::string ip;
  uint32_t port;
  uint64_t to_UInt64() const;
  int32_t from_UInt64(uint64_t UInt64_ip_port) ;
} ip_port_t;

#endif //UDPTUN_TOOL_H
