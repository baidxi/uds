#pragma once

#include <stdint.h>
#include <stddef.h>

class uds_server;

struct uds_service_message {
    uint8_t sid;
    uint8_t sub_id;
    uint8_t param[];
};
class uds_service {
public:
    uds_service();
    ~uds_service();
    void bind(uds_server *svr);
    virtual int handle_msg(void *buf, size_t size) = 0;
    int response(void *buf, size_t len);
private:
    uds_server *server;
};



class diag_session_control:public uds_service
{
public:
    diag_session_control() = default;
    ~diag_session_control() = default;
    int handle_msg(void *buf, size_t size) override;
};

class ecu_reset:public uds_service
{
public:
    ecu_reset() = default;
    ~ecu_reset() = default;
    int handle_msg(void *buf, size_t size) override;
};