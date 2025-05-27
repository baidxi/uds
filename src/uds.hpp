#pragma once

#include <map>
#include <memory>
#include <vector>

#include "can.hpp"
#include "ecu.hpp"

class uds : public ecu_hw
{
public:
    uds();
    ~uds();
    void loop_run();
    int handle_msg(void *buf, size_t size) override;
    int register_ecu(int fd, std::shared_ptr<ecu_hw> ecu);
private:
    int epoll_fd;
    bool running;
    std::vector<std::shared_ptr<ecu_hw>> ecu_list;
};