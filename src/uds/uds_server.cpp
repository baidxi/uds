#include <sys/epoll.h>
#include <unistd.h>
#include <linux/can/raw.h>

#include <thread>

#include "uds_server.hpp"
#include "uds_service.hpp"
#include "can.hpp"

uds_server::uds_server(int id)
{
    epoll_event event;

    epoll_fd = epoll_create(100);
    if (epoll_fd < 0) {
        fprintf(stderr, "epoll fd err\n");
        return;
    }

    // event.data.ptr = this;
    // event.events = EPOLLIN;

    // if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, getfd(), &event)) {
    //     fprintf(stderr, "epoll err\n");
    //     return;
    // }

    this->id = id;
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
                }).join();
            }
        }
    }
}

int uds_server::handle_msg(void *buf, size_t size)
{
    uds_service_message *msg = (uds_service_message *)buf;

    fprintf(stdout, "server handle msg\n");

    if (processor) {
        processor->handle_msg(buf, size);
    } else {
        std::shared_ptr<uds_service> service = services[msg->sid];
        if (service) {
            service->handle_msg(buf, size);
            processor = service;
        } else {
            fprintf(stderr, "unk sid 0x%02x\n", msg->sid);
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
    svr->bind(this);
    services.insert({sid, svr});
    fprintf(stdout, "Service 0x%02x registered\n", sid);
    return 0;
}

void uds_server::service_init()
{
    int fd = can_socket_new();
    
    if (fd < 0) {
        fprintf(stderr, "new can fd err\n");
        return;
    }

    if (can_hw::bind(fd, can_hw::get_ifname())) {
        fprintf(stderr, "service bind err\n");
        return;
    }

    epoll_event ev;
    ev.data.ptr = this;
    ev.events = EPOLLIN;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev)) {
        fprintf(stderr, "service init err\n");
        return ;
    }

    register_service(0x10, std::make_shared<diag_session_control>());
    register_service(0x11, std::make_shared<ecu_reset>());
}

int uds_server::response(can_frame *frame, size_t len)
{
    frame->can_id = id;
    return can_hw::send(frame, len);
}