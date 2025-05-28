#pragma once

#include <string>

#include <stddef.h>

class can_hw {
    int can_sock_fd;
    int netlink_fd;
    std::string name;
    int ifchange(const std::string &ifname, int state);
public:
    can_hw();
    ~can_hw();
    int send(const void *buf, size_t len);
    size_t recv(void *buf, size_t size);
    int init(const std::string &ifname, uint32_t bitrate);
    int up(const std::string &ifname);
    int down(const std::string &ifname);
    int bind(const std::string &ifname);
    int bind(int fd, const std::string &ifname);
protected:
    std::string get_ifname() {
        return name;
    }
};

int can_socket_new();