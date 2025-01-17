#include "cserver.h"
#include "defines.h"
#include <iostream>

int main()
{
    try {
        boost::asio::io_context ioc;
        CServer server(ioc, PORT);
        ioc.run();
    } catch (std::exception& e) {
        std::cerr << "Server init false :" << e.what() << std::endl;
    }
}
