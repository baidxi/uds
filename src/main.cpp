#include "uds_server.hpp"

int main()
{
    auto uds = std::make_shared<uds_server>(0x123);
    int fd;

    // if (uds->bind("can0")) {
    //     fprintf(stderr, "bind err\n");
    //     goto out;
    // }
    uds->down("can0");
    uds->init("can0", 50000);
    uds->up("can0");

    // fd = uds->bind("can0");
    // if (fd > 0) {
    //     uds->register_ecu(fd, uds);
    // }

    uds->service_init();

    uds->loop_run();

    return 0;
out:
    return -1;
}