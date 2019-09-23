#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <glog/logging.h>
#include <string.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include "connection_manager.h"
#include "tool.h"

const size_t LISTEN_PORT = 9999;
const std::string local_ip = "127.0.0.1";
const size_t local_port = 9999;
const std::string remote_ip = "127.0.0.1";
const size_t remote_port = 9877;
// const std::string remote_ip = "107.182.186.209";
// const size_t remote_port = 9999;
// const size_t remote_port = 21259;

void run() {
    int epoll_fd = -1;
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        LOG(ERROR) << "create epoll failed error:" << strerror(errno);
        return;
    }
    int local_listen_fd = -1;
    new_listen_socket(local_ip, local_port, local_listen_fd);
    const int max_events = 4096;
    struct epoll_event ev, events[max_events];
    ev.events = EPOLLIN;
    ev.data.fd = local_listen_fd;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, local_listen_fd, &ev);
    if (ret != 0) {
        close(local_listen_fd);
        LOG(INFO) << "add local_udp_listen_fd failed, error:" << strerror(errno);
        return;
    }
    int remote_connected_fd = -1;
    ret = new_connected_socket(remote_ip, remote_port, remote_connected_fd);
    if (ret != 0) {
        LOG(ERROR) << "failed to connect to remote ip:" << remote_ip << " port"
                   << remote_port;
        close(local_listen_fd);
        return;
    }
    ev.data.fd = remote_connected_fd;
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, remote_connected_fd, &ev);
    if (ret != 0) {
        close(local_listen_fd);
        close(remote_connected_fd);
        LOG(INFO) << "add remote_udp_connected_fd failed, error:"
                  << strerror(errno);
        return;
    }
    ConnectionManager connection_manager(local_listen_fd, remote_connected_fd);
    while (1) {
        int nfds = epoll_wait(epoll_fd, events, max_events, -1);
        if (nfds < 0) {
            if (errno == EINTR) {
                LOG(WARNING) << "epoll interrupted by signal continue";
                continue;
            } else {
                LOG(ERROR) << "epoll_wait return " << nfds
                           << " error:" << strerror(errno);
                break;
            }
        }
        for (size_t index = 0; index < nfds; ++index) {
            if (events[index].data.fd == remote_connected_fd) {
                ///接收到来自远端的消息
                ret = -1;
                bzero(connection_manager.recv_buf_, sizeof(connection_manager.recv_buf_));
                ret = recv(remote_connected_fd, connection_manager.recv_buf_, sizeof(connection_manager.recv_buf_), 0);
                if (ret > 0) {
                    connection_manager.recv_buf_len_ = ret;
                    ///通知local_connection_manager有数据到达,需要将数据回送到之前请求该数据的地址
                    connection_manager.SendMesgToLocal();
                    /// TODO
                } else {
                    if (ret == 0) {
                        ///对于udp协议来说,由于其无连接性,所以ret为0是正常情况,对于tcp来说ret=0则代表连接断开
                    } else {
                        if (errno == EAGAIN) {
                            LOG(INFO) << "try again";
                            continue;
                        }
                        LOG(WARNING) << "receive data failed error:"
                                     << strerror(errno);
                    }
                    // epoll_ctl(epoll_fd, EPOLL_CTL_DEL, remote_connected_fd, nullptr);
                    ///只要不调用close,内核中由于之前对remote_connected_fd调用connect而保存的对端ip和port都一直存在
                    // close(remote_connected_fd);
                }
            } else if (events[index].data.fd == local_listen_fd) {
                ///接收到来自本地的消息
                ret = -1;
                bzero(connection_manager.send_buf_, BUF_SIZE);
                sockaddr_in addr;
                socklen_t slen = sizeof(addr);
                ret = recvfrom(local_listen_fd, connection_manager.send_buf_,
                               BUF_SIZE, 0, (sockaddr *) &addr, &slen);
                if (ret > 0) {
                    connection_manager.send_buf_len_ = ret;
                    uint64_t UInt64_ip_port = 0;
                    ip_port_netorder2uint64(addr.sin_addr.s_addr, addr.sin_port, UInt64_ip_port);
                    ///这里查看local_connection_manager中是否存在该连接,不存在的话就新增连接
                    if (!connection_manager.Exist(UInt64_ip_port)) {
                        connection_manager.AddConnection(UInt64_ip_port);
                    }
                    ip_port_t local_ip_port;
                    local_ip_port.from_UInt64(UInt64_ip_port);
                    LOG(INFO) << "receive from ip:" << local_ip_port.ip << " port:" << local_ip_port.port << " data:"
                              << connection_manager.send_buf_;
                    ///通知remote_connection_manager有消息需要发送给远端
                    connection_manager.SendMesgToRemote(UInt64_ip_port);
                } else {
                    if (ret == 0) {
                        ///对于udp协议来说,由于其无连接性,所以ret为0是正常情况,对于tcp来说ret=0则代表连接断开
                    } else {
                        if (errno == EAGAIN) {
                            continue;
                        }
                        LOG(WARNING) << "receive data failed error:" << strerror(errno);
                    }
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, local_listen_fd, nullptr);
                    close(local_listen_fd);
                }
            }
        }
    }
}

int main(int argc, char const *argv[]) {
    google::InitGoogleLogging("INFO");
    FLAGS_logtostderr = true;
    run();
    return 0;
}
