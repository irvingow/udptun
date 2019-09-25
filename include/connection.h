//
// Created by lwj on 2019/9/20.
//

#ifndef UDPTUN_CONNECTION_H
#define UDPTUN_CONNECTION_H
#include <boost/noncopyable.hpp>
#include "tool.h"
#include <unordered_map>

class Connection : public boost::noncopyable {
 public:
  Connection(const ip_port_t& ip_port, const int32_t& local_listen_fd);
  int32_t AddConnectedFdAndConnId(const int32_t& connected_fd, const uint32_t& conn_id);
  bool Exist(const uint32_t& conn_id) const;
  ///接收到来自Server的数据,需要发送给对应client
  void GotDataFromServer();
  ///接收到来自client的数据,需要将数据发送给server
  void GotDataFromClient(const uint32_t& conn_id);
  ip_port_t connection_ip_port() const { return client_ip_port_; }
  char recv_buf_[BUF_SIZE];
  int32_t recv_buf_len_;
  char send_buf_[BUF_SIZE];
  int32_t send_buf_len_;
 private:
  int32_t listen_fd_;
  std::unordered_map<int32_t , uint32_t > connected_fd2conn_id_hashmap_;
  std::unordered_map<uint32_t , int32_t > conn_id2connected_fd_hashmap_;
  ip_port_t client_ip_port_;
};

#endif //UDPTUN_CONNECTION_H
