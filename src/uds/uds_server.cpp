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

struct proto_ctrl_info {
    union {
        struct {
            uint8_t len:4;
            uint8_t sid;
            uint8_t __;
        }__attribute__((packed)) sf;
        struct {
            unsigned len:12;
            uint8_t _:4;
        }__attribute__((packed)) ff;
        struct {
            uint8_t sn:4;
            uint8_t _;
            uint8_t __;
        }__attribute__((packed)) cf;
        struct {
            uint8_t fs:4;
            uint8_t bs;
            uint8_t stmin;
        }__attribute__((packed)) fc;
    };
   uint8_t type:4;
}__attribute__((packed));

int uds_server::handle_msg(void *buf, size_t size)
{
    can_frame *frame = (can_frame *)buf;
    proto_ctrl_info *info = (proto_ctrl_info *)frame->data;
    uint8_t *payload = frame->data;
    std::shared_ptr<uds_service> svr;
    uint8_t len, type;

#define SF  0
#define FF  1
#define CF  2
#define FC  3

    if (frame->can_id < 0x700 && frame->can_id > 0x7df) {
        return 0;
    }

    fprintf(stdout, "server handle msg:%08x, %d, type:%d,%d\n", frame->can_id, frame->len, info->type, sizeof(*info));

    switch(info->type) {
        case SF:
            printf("len:%d, sid:0x%02x\n", info->sf.len, info->sf.sid);
            svr = services[info->sf.sid];
            if (svr)
                svr->handle_msg(frame->can_id, payload + 2, info->sf.len -1);
        
            break;
        case FF:
            break;
        case CF:
            break;
        case FC:
            break;
        default:
            break;
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
    return can_hw::send(frame, len);
}