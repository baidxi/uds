#include "uds_server.hpp"

int main()
{
    auto uds = std::make_shared<uds_server>();

    uds->bind("can0");
    uds->init("can0", 125000);
    uds->up("can0");

    uds->service_init();

    uds->loop_run();

    return 0;
}