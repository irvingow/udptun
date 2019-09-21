//
// Created by lwj on 2019/9/20.
//
#include "connection.h"
#include <cstring>

Connection::Connection(const ip_port_t &ip_port) {
    connection_ip_port_ = ip_port;
}

Connection::Connection(const uint64_t &UInt64_ip_port) {
    connection_ip_port_.from_UInt64(UInt64_ip_port);
}

