#include <iostream>
#include <config/Logger.hpp>

int main()
{
    std::cout << "Smart Greenhouse server: build successful!\n";

    INIT_LOGGER_DEFAULT();

    return 0;
}
