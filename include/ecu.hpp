#pragma once
#include <stddef.h>

#include "can.hpp"

class ecu_hw:public can_hw
{
public:
    ecu_hw();
    ~ecu_hw();
    virtual int handle_msg(void *buf, size_t size) = 0;
};