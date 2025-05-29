#pragma once

#include <stdint.h>
#include <stddef.h>

class uds_server;

struct negative_response {
    uint8_t sid;
    uint8_t code;
};
class uds_service {
public:
    uds_service();
    ~uds_service();
    void bind(uds_server *svr);
    virtual int handle_msg(int can_id, void *buf, size_t size) = 0;
private:
    uds_server *server;
protected:
    int response(int can_id, void *buf, size_t len);
};



class diag_session_control:public uds_service
{
public:
    diag_session_control() = default;
    ~diag_session_control() = default;
    int handle_msg(int can_id, void *buf, size_t size) override;
private:
    uint8_t mode;
};

class ecu_reset:public uds_service
{
public:
    ecu_reset() = default;
    ~ecu_reset() = default;
    int handle_msg(int can_id, void *buf, size_t size) override;
};