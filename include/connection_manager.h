#ifndef CONNECTION_MANAGER_H_
#define CONNECTION_MANAGER_H_

#include <boost/noncopyable.hpp>
#include <set>
#include <unordered_map>
#include <memory>
#include <unordered_set>
#include "connection.h"
#include "tool.h"

class ConnectionManager : public boost::noncopyable {
 public:
  ConnectionManager(const int32_t &epoll_fd,
                    const ip_port_t &remote_ip_port,
                    const int32_t &listen_fd,
                    const int32_t &connected_fd);
  int32_t AddConnection(const ip_port_t &ip_port,
                        const int32_t &listen_fd,
                        const int32_t &connected_fd,
                        const uint32_t &conn_id);
  int32_t ConnectionAddConnectedFd(const ip_port_t &ip_port, const int32_t &connected_fd, const uint32_t &conn_id);
  bool Exist(const uint64_t &uint64_ip_port);
  ///udptun server调用,接收到来自udptun_client的数据
  void GotDataWithConnIdFromClient();
  ///udptun client,接收到来自client的数据
  void GotDataWithoutConnIdFromClient();
  ///udptun client调用,接收到来自udptun_server的数据
  void GotDataWithConnIdFromServer();
  ///udptun server,接收到来自server的数据
  void GotDataWithoutConnIdFromServer(const int32_t &connected_fd);
  char recv_buf_[BUF_SIZE + 4];
  uint32_t recv_buf_len_;
  char send_buf_[BUF_SIZE + 4];
  uint32_t send_buf_len_;
 private:
  std::unordered_map<uint64_t, std::shared_ptr<Connection>> uint64_ip_port2conn_hashmap_;
  std::unordered_map<uint64_t, std::unordered_set<uint32_t> > uint64_ip_port2conn_ids_hashmap_;
  std::unordered_map<int32_t, uint64_t> connected_fd2uint64_ip_port_hashmap_;///for udptun server only
  std::unordered_map<int32_t, uint32_t> connected_fd2conn_id_hashmap_;///for udptun server only
  std::unordered_map<uint32_t, uint64_t> conn_id2uin64_ip_port_hashmap_;///for udptun client only
  int32_t epoll_fd_;
  int32_t listen_fd_;
  int32_t connected_fd_;///for udptun server this is -1, for udptun client this is valid, connects to the remote udptun server
  ip_port_t server_ip_port_;
};

#endif