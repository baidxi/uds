#include "uds.hpp"

int main()
{
    auto _uds = new uds();

    _uds->bind("can0");
    _uds->init("can0", 125000);
    _uds->up("can0");

    delete _uds;

    return 0;
}