//
// Created by lwj on 2019/9/20.
//
#include "connection.h"
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glog/logging.h>

Connection::Connection(const ip_port_t &ip_port,
                       const uint32_t &unique_conn_id,
                       const int32_t &local_listen_fd,
                       const int32_t &remote_connected_fd) {
    connection_ip_port_ = ip_port;
    unique_connection_id_ = unique_conn_id;
    local_listen_fd_ = local_listen_fd;
    remote_connected_fd_ = remote_connected_fd;
    bzero(recv_buf_, BUF_SIZE);
    recv_buf_len_ = 0;
    bzero(send_buf_, BUF_SIZE);
    send_buf_len_ = 0;
}

Connection::Connection(const uint64_t &UInt64_ip_port,
                       const uint32_t &unique_conn_id,
                       const int32_t &local_listen_fd,
                       const int32_t &remote_connected_fd) {
    connection_ip_port_.from_UInt64(UInt64_ip_port);
    unique_connection_id_ = unique_conn_id;
    local_listen_fd_ = local_listen_fd;
    remote_connected_fd_ = remote_connected_fd;
    bzero(recv_buf_, BUF_SIZE);
    recv_buf_len_ = 0;
    bzero(send_buf_, BUF_SIZE);
    send_buf_len_ = 0;
}

void Connection::SendDataToLocal() {
    sockaddr_in remote_addr;
    remote_addr.sin_addr.s_addr = inet_addr(connection_ip_port_.ip.c_str());
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(connection_ip_port_.port);
    auto ret = sendto(local_listen_fd_, recv_buf_,
                      recv_buf_len_, 0,
                      (sockaddr *) &remote_addr,
                      sizeof(remote_addr));
    if (ret < 0) {
        LOG(ERROR) << "send data to local_addr ip:" << connection_ip_port_.ip << " port:" << connection_ip_port_.port
                   << " error:"
                   << strerror(errno);
    }
}

void Connection::SendDataToRemote() {
    int send_ret =
        send(remote_connected_fd_, send_buf_, send_buf_len_, 0);
    if (send_ret < 0) {
        ///理论上remote_connection_manager的UInt64_ip_port_Conn_map_中有且只有一个连接,那就是连接远端的连接
        LOG(ERROR) << "send data to remote failed error:"
                   << strerror(errno);
    }
}

