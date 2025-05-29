#include <linux/can/raw.h>
#include <cstring>

#include "uds_service.hpp"
#include "uds_server.hpp"

uds_service::uds_service()
{

}

uds_service::~uds_service()
{
    server = nullptr;
}

void uds_service::bind(uds_server *svr)
{
    server = svr;
}

int uds_service::response(int can_id, void *buf, size_t len)
{
    can_frame frame;
    frame.can_id = can_id;
    frame.len = len;
    memcpy(frame.data, buf, len);

    return server->response(&frame, sizeof(frame));
}

int diag_session_control::handle_msg(int can_id, void *buf, size_t size)
{
    uint8_t mode = *(uint8_t *)buf;
    char _buf[3];
    fprintf(stdout, "diag_session_control handle msg:%d, mode:%d\n", size, mode);

    _buf[0] = 0x02;
    _buf[1] = 0x50;
    _buf[2] = mode;
    response(can_id + 1, _buf, 3);

    this->mode = mode;

    return 0;
}

int ecu_reset::handle_msg(int can_id, void *buf, size_t size)
{
    return 0;
}