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
    struct epoll_event events[max_events];
    auto ret = AddEvent2Epoll(epoll_fd, local_listen_fd, EPOLLIN);
    if (ret != 0) {
        close(local_listen_fd);
        LOG(INFO) << "add local_udp_listen_fd to epoll failed, error:" << strerror(errno);
        return;
    }
    int remote_connected_fd = -1;
    ret = new_connected_socket(remote_ip, remote_port, remote_connected_fd);
    if (ret != 0) {
        LOG(ERROR) << "failed to create remote_connected_fd remote_ip:" << remote_ip << " port"
                   << remote_port;
        close(local_listen_fd);
        return;
    }
    ret = AddEvent2Epoll(epoll_fd, remote_connected_fd, EPOLLIN);
    if (ret != 0) {
        close(remote_connected_fd);
        LOG(INFO) << "add local_udp_listen_fd failed, error:" << strerror(errno);
        return;
    }
    ip_port_t remote_ip_port;
    remote_ip_port.ip = remote_ip;
    remote_ip_port.port = remote_port;
    ConnectionManager connection_manager(epoll_fd, remote_ip_port, local_listen_fd, remote_connected_fd);
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
                connection_manager.GotDataWithConnIdFromServer();
            } else if (events[index].data.fd == local_listen_fd) {
                ///接收到来自本地的消息
                connection_manager.GotDataWithoutConnIdFromClient();
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
