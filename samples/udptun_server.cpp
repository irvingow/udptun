#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <glog/logging.h>
#include <string.h>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <unordered_set>
#include "connection_manager.h"
#include "parse_config.h"
#include "tool.h"

void run(const std::string& config_file_path) {
    SystemConfig* instance = SystemConfig::GetInstance(config_file_path);
    auto system_config = instance->system_config();
    if(!system_config->parse_flag){
        LOG(ERROR)<<"failed to parse config file";
        return;
    }
    const std::string local_ip = system_config->listen_ip;
    const size_t local_port = system_config->listen_port;
    const std::string remote_ip = system_config->remote_ip;
    const size_t remote_port = system_config->remote_port;
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
    ip_port_t remote_ip_port;
    remote_ip_port.ip = remote_ip;
    remote_ip_port.port = remote_port;
    ///对于udptun_server端来说,remote_connected_fd是不需要一开始就创建的(而是根据udptun_client
    /// 客户端中本地连接的个数来确定的),所以初始化为-1,避免被使用)
    ConnectionManager connection_manager(epoll_fd, remote_ip_port, local_listen_fd, remote_connected_fd);
    std::unordered_set<int32_t> remote_connected_fds_set;
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
            if (events[index].data.fd != local_listen_fd) {
                ///接收到来自服务器的消息
                connection_manager.GotDataWithoutConnIdFromServer(events[index].data.fd);
            } else if (events[index].data.fd == local_listen_fd) {
                ///接收到来自udptun客户端发来的的消息
                connection_manager.GotDataWithConnIdFromClient();
            }
        }
    }
}

int main(int argc, char const *argv[]) {
    google::InitGoogleLogging("INFO");
    FLAGS_logtostderr = true;
    if(argc != 2){
        LOG(ERROR)<<"usage:"<<argv[0]<<" config_json_path";
        return 0;
    }
    const std::string config_file_path(argv[1]);
    run(config_file_path);
    return 0;
}
