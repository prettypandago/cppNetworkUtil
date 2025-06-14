#include <iostream>

#include "init.h"
#include "cppNetworkUtil.h"

int port = 80;

void process(std::string recv_data, SOCKET client_socket)
{
    cppNetworkUtil client;

    client.sendData(recv_data, client_socket);
}

int main(int argc, char **argv)
{
    cppNetworkUtil server;

    SOCKET server_socket;
    try
    {
        server_socket = server.start(port);
    }
    catch (const std::exception &e)
    {
        if (IS_DEBUG)
        {
            std::cerr << e.what() << '\n';
        }
    }
    printf("Listening on 0.0.0.0:%d\n", port);

    server.exec(process, 20, server_socket);

    return 0;
}