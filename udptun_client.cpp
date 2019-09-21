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

const size_t LISTEN_PORT = 9999;
const std::string local_ip = "127.0.0.1";
const size_t local_port = 9999;
const std::string remote_ip = "127.0.0.1";
const size_t remote_port = 9877;
// const std::string remote_ip = "107.182.186.209";
// const size_t remote_port = 9999;
// const size_t remote_port = 21259;
// const size_t BUFSIZE = 1024;

int set_non_blocking(const int &fd) {
    int opts = -1;
    opts = fcntl(fd, F_GETFL);

    if (opts < 0) {
        LOG(ERROR) << "get socket status failed, fd:" << fd
                   << " error:" << strerror(errno);
        return -1;
    }
    opts = opts | O_NONBLOCK;
    if (fcntl(fd, F_SETFL, opts) < 0) {
        LOG(ERROR) << "set socket non_blocking failed, fd:" << fd
                   << " error:" << strerror(errno);
        return -1;
    }
}

int new_listen_socket(const std::string &ip, const size_t &port, int& fd) {
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == -1) {
        LOG(ERROR) << "create new socket failed" << strerror(errno);
        return -1;
    }
    struct sockaddr_in local_addr = {0};
    socklen_t slen = sizeof(local_addr);
    local_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *) &local_addr, slen) == -1) {
        LOG(ERROR) << "socket bind error port:" << port
                   << " error:" << strerror(errno);
        return -1;
    }
    set_non_blocking(fd);
    LOG(INFO) << "local socket listen_fd:" << fd;
}

int new_connected_socket( const std::string &remote_ip,
                         const size_t &remote_port,int &fd) {
    LOG(INFO) << "remote ip:" << remote_ip << " port:" << remote_port;
    struct sockaddr_in remote_addr_in = {0};
    socklen_t slen = sizeof(remote_addr_in);
    remote_addr_in.sin_addr.s_addr = inet_addr(remote_ip.c_str());
    remote_addr_in.sin_family = AF_INET;
    remote_addr_in.sin_port = htons(remote_port);

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        LOG(ERROR) << "create new socket failed" << strerror(errno);
        return -1;
    }
    int ret = connect(fd, (struct sockaddr *) &remote_addr_in, slen);
    if (ret < 0) {
        ///实际上connect基本不会失败,因为内核只做一些基本的检查,比如参数是否合法,ip是否有效等等,
        ///之后内核就创建相应数据结构,并且返回,并没有真的去连接那个ip和port
        LOG(ERROR) << "failed to establish connection to remote, error:"
                   << strerror(errno);
        close(fd);
        return -1;
    }
    set_non_blocking(fd);
    LOG(INFO) << "create new remote connection udp_fd:" << fd;
    return 0;
}

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

    ConnectionManager remote_connection_manager(remote_connected_fd);
    ip_port_t remote_ip_port;
    remote_ip_port.ip = remote_ip;
    remote_ip_port.port = remote_port;
    remote_connection_manager.AddConnection(remote_ip_port);
    ConnectionManager local_connection_manager(local_listen_fd);
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
                bzero(local_connection_manager.buf_, sizeof(local_connection_manager.buf_));
                ret = recv(remote_connected_fd, local_connection_manager.buf_, BUF_SIZE, 0);
                if (ret > 0) {
                    local_connection_manager.buf_len_ = ret;
                    LOG(INFO) << "receive data:"
                              << local_connection_manager.buf_;
                    ///通知local_connection_manager有数据到达,需要将数据回送到之前请求该数据的地址
                    local_connection_manager.SendMesgToLocal();
                    /// TODO
                } else {
                    if (ret == 0) {
                        ///对于udp协议来说,由于其无连接性,所以ret为0是正常情况,对于tcp来说ret=0则代表连接断开
                    } else {
                        if (errno == EAGAIN) {
                            LOG_FIRST_N(INFO, 20) << "try again";
                            continue;
                        }
                        // LOG(WARNING) << "receive data failed error:" << strerror(errno);
                        LOG_FIRST_N(WARNING, 10) << "receive data failed error:"
                                                 << strerror(errno);
                    }
                    // epoll_ctl(epoll_fd, EPOLL_CTL_DEL, remote_connected_fd, nullptr);
                    ///只要不调用close,内核中由于之前对remote_connected_fd调用connect而保存的对端ip和port都一直存在
                    // close(remote_connected_fd);
                }
            } else if (events[index].data.fd == local_listen_fd) {
                ///接收到来自本地的消息
                ret = -1;
                bzero(remote_connection_manager.buf_, BUF_SIZE);
                sockaddr addr;
                socklen_t slen = sizeof(addr);
                ret = recvfrom(local_listen_fd, remote_connection_manager.buf_,
                               BUF_SIZE, 0, &addr, &slen);
                if (ret > 0) {
                    remote_connection_manager.buf_len_ = ret;
                    sockaddr_in *temp = (sockaddr_in *) &addr;
                    uint64_t  UInt64_ip_port = 0;
                    ip_port_netorder2uint64(temp->sin_addr.s_addr, temp->sin_port, UInt64_ip_port);
                    ///这里查看local_connection_manager中是否存在该连接,不存在的话就新增连接
                    if(!local_connection_manager.Exist(UInt64_ip_port)){
                        local_connection_manager.AddConnection(UInt64_ip_port);
                    }
                    LOG(INFO) << "receive data:"
                              << remote_connection_manager.buf_;
                    ///通知remote_connection_manager有消息需要发送给远端
                    remote_connection_manager.SendMesgToRemote(UInt64_ip_port);
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
