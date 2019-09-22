#ifndef CONNECTION_MANAGER_H_
#define CONNECTION_MANAGER_H_

#include <boost/noncopyable.hpp>
#include <set>
#include <unordered_map>
#include <memory>
#include "connection.h"
#include "tool.h"

class ConnectionManager : public boost::noncopyable {
 public:
  explicit ConnectionManager(const uint32_t &fd) : fd_(fd), buf_(), buf_len_(0) {}
  int32_t AddConnection(const ip_port_t &ip_port);
  int32_t AddConnection(const uint64_t &uint64_ip_port);
  int32_t RemoveConnection(const ip_port_t &ip_port);
  bool Exist(const ip_port_t &ip_port) const;
  bool Exist(const uint64_t &uint64_ip_port) const;
  ///接收到来自远端的数据,将数据回送到请求该数据的地址
  void SendMesgToLocal();
  ///通知ConnectionManager已经有新的数据到达,可以选择发送数据,注意参数并不是远端地址,而是收到的消息的来源地址
  void SendMesgToRemote(const uint64_t& uint64_ip_port);
  ///将connection对应的唯一id数据写入到即将要发送的数据中去
  int32_t PutConnIdToData(const uint64_t & uint64_ip_port);
  ///从数据包中获得这个数据包的应该被发送到的地址
  int32_t GetIpPortFromData(ip_port_t &ip_port);
  char buf_[BUF_SIZE+4];
  uint32_t buf_len_;
 private:
  ///保存所有ip和port转化为的uint64_t值及其映射到的connection
  std::unordered_map<uint64_t, std::shared_ptr<Connection>> UInt64_ip_port_Conn_map_;
  std::unordered_map<uint32_t , std::shared_ptr<Connection>> unique_conn_id_map_;
  uint32_t fd_;
};

#endif