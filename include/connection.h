//
// Created by lwj on 2019/9/20.
//

#ifndef UDPTUN_CONNECTION_H
#define UDPTUN_CONNECTION_H
#include <boost/noncopyable.hpp>
#include "tool.h"

class Connection : public boost::noncopyable {
 public:
  explicit Connection(const ip_port_t& ip_port);
  explicit Connection(const uint64_t & UInt64_ip_port);
  ip_port_t connection_ip_port() const {return connection_ip_port_;}
 private:
  ip_port_t connection_ip_port_;
};

#endif //UDPTUN_CONNECTION_H
