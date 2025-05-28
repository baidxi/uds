#include <sys/epoll.h>
#include <unistd.h>

#include <thread>

#include "uds_server.hpp"
#include "uds_service.hpp"
#include "can.hpp"

uds_server::uds_server()
{
    epoll_event event;

    epoll_fd = epoll_create(100);
    if (epoll_fd < 0) {
        fprintf(stderr, "epoll fd err\n");
        return;
    }

    event.data.ptr = this;
    event.events = EPOLLIN;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, getfd(), &event)) {
        fprintf(stderr, "epoll err\n");
        return;
    }
}

uds_server::~uds_server()
{
    if (epoll_fd > 0)
        close(epoll_fd);
}

void uds_server::loop_run()
{
    epoll_event events[100];
    epoll_event *event;
    ecu_hw *ecu;
    running = true;
    int ret;

    while(running) {
        ret = epoll_wait(epoll_fd, events, 100, -1);
        if (ret > 0) {
            for (int i = 0; i < ret; i++) {
                event = &events[i];
                ecu = (ecu_hw *)event->data.ptr;
                std::thread([ecu]{
                    char buf[1024];
                    int size = ecu->recv(buf, sizeof(buf));
                    if (size > 0) {
                        ecu->handle_msg(buf, size);
                    }
                });
            }
        }
    }
}

int uds_server::handle_msg(void *buf, size_t size)
{
    uds_service_message *msg = (uds_service_message *)buf;

    if (processor) {
        processor->handle_msg(buf, size);
    } else {
        std::shared_ptr<uds_service> service = services[msg->type];
        if (service) {
            service->handle_msg(buf, size);
            processor = service;
        } else {

        }
    }

    return 0;
}

int uds_server::register_ecu(int fd, std::shared_ptr<ecu_hw> ecu)
{
    epoll_event ev;
    ev.data.ptr = ecu.get();
    ev.events = EPOLLIN;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev)) {
        fprintf(stderr, "add ecu err\n");
        return -1;
    }

    ecu_list.push_back(ecu);

    return 0;
}

int uds_server::register_service(uint8_t sid, std::shared_ptr<uds_service> svr)
{
    services.insert({sid, svr});
    return 0;
}

void uds_server::service_init()
{
    register_service(0x10, std::make_shared<diag_session_control>());
    register_service(0x11, std::make_shared<ecu_reset>());
}