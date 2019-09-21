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
  int32_t AddConnection(const ip_port_t& ip_port);
  int32_t RemoveConnection(const ip_port_t& ip_port);
  bool Exist(const ip_port_t& ip_port) const;
private:
  ///保存所有ip和port转化为的uint64_t值及其映射到的connection
  std::unordered_map<uint64_t , std::shared_ptr<Connection>> UInt64_ip_port_Conn_map_;
};

#endif