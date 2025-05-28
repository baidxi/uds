#include "uds_service.hpp"

uds_service::uds_service()
{

}

uds_service::~uds_service()
{

}

void uds_service::bind(uds_server *svr)
{
    server = svr;
}

int uds_service::request(uint8_t sid, void *buf, size_t size)
{
    return 0;
}

int diag_session_control::handle_msg(void *buf, size_t size)
{
    return 0;
}

int ecu_reset::handle_msg(void *buf, size_t size)
{
    return 0;
}