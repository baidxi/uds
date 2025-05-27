#include <sys/epoll.h>
#include <unistd.h>

#include <thread>

#include "uds.hpp"
#include "can.hpp"

uds::uds()
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

uds::~uds()
{
    if (epoll_fd > 0)
        close(epoll_fd);
}

void uds::loop_run()
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

int uds::handle_msg(void *buf, size_t size)
{
    return 0;
}

int uds::register_ecu(int fd, std::shared_ptr<ecu_hw> ecu)
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