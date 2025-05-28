#pragma once

#include "uds_server.hpp"

class uds_service {
public:
    uds_service();
    ~uds_service();
    void bind(std::shared_ptr<uds_server> svr);
    virtual int handle_msg(void *buf, size_t size) = 0;
    int request(uint8_t sid, void *buf, size_t len);
private:
    std::shared_ptr<uds_server> server;
};

struct uds_service_message {
    uint8_t type;
    uint8_t param[];
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