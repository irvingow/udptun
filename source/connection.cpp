//
// Created by lwj on 2019/9/20.
//
#include "connection.h"
#include <cstring>

Connection::Connection(const ip_port_t &ip_port, const uint32_t& unique_conn_id) {
    connection_ip_port_ = ip_port;
    unique_connection_id_ = unique_conn_id;
}

Connection::Connection(const uint64_t &UInt64_ip_port, const uint32_t& unique_conn_id) {
    connection_ip_port_.from_UInt64(UInt64_ip_port);
    unique_connection_id_ = unique_conn_id;
}

