//
// Created by lwj on 2019/9/20.
//

#ifndef UDPTUN_CONNECTION_H
#define UDPTUN_CONNECTION_H
#include <boost/noncopyable.hpp>
#include "tool.h"

class Connection : public boost::noncopyable {
 public:
  explicit Connection(const ip_port_t &ip_port, const uint32_t &unique_conn_id, const uint32_t& local_listen_fd, const uint32_t& remote_connected_fd);
  explicit Connection(const uint64_t &UInt64_ip_port, const uint32_t &unique_conn_id, const uint32_t& local_listen_fd, const uint32_t& remote_connected_fd);
  ///接收到来自远端的数据,需要发送给对应本地连接
  void SendDataToLocal();
  ///接收到来自本地的数据,需要将数据发送给远端
  void SendDataToRemote();
  ip_port_t connection_ip_port() const { return connection_ip_port_; }
  uint32_t unique_connection_id() const { return unique_connection_id_; }
  char recv_buf_[BUF_SIZE];
  int32_t recv_buf_len_;
  char send_buf_[BUF_SIZE];
  int32_t send_buf_len_;
 private:
  uint32_t local_listen_fd_;
  uint32_t remote_connected_fd_;
  ip_port_t connection_ip_port_;
  uint32_t unique_connection_id_;
};

#endif //UDPTUN_CONNECTION_H
