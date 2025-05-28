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

int uds_service::response(void *buf, size_t size)
{
    can_frame frame;

    memcpy(&frame.data, buf, size);

    return server->response(&frame, sizeof(frame));
}

int diag_session_control::handle_msg(void *buf, size_t size)
{
    uds_service_message *msg = (uds_service_message *)buf;
    fprintf(stdout, "diag_session_control handle msg\n");
    uint8_t *param;
    if (size < 2) {
        msg->sid = 0x7f;
        msg->sub_id = 0x10;
        param = msg->param;
        param[0] = 0x13;
        response(msg, 0x4);
    } else {

    }

    return 0;
}

int ecu_reset::handle_msg(void *buf, size_t size)
{
    return 0;
}