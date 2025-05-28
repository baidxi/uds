#pragma once

#include <map>
#include <memory>
#include <vector>

struct can_frame;

#include "can.hpp"
#include "ecu.hpp"
#include "uds_service.hpp"

class uds_server : public ecu_hw
{
public:
    explicit uds_server(int id);
    ~uds_server();
    void loop_run();
    int handle_msg(void *buf, size_t size) override;
    int register_ecu(int fd, std::shared_ptr<ecu_hw> ecu);
    void leave_service() {
        processor = nullptr;
    }
    void service_init();
    int response(can_frame *frame, size_t size);
private:
    int epoll_fd;
    bool running;
    std::vector<std::shared_ptr<ecu_hw>> ecu_list;
    std::map<uint8_t, std::shared_ptr<uds_service>> services;
    std::shared_ptr<uds_service> processor;
    int register_service(uint8_t sid, std::shared_ptr<uds_service> svr);
    int id;
};
