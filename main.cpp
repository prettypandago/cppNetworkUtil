#include <iostream>

#include "cppNetworkUtil.h"

int port = DEFAULT_SERVER_PORT;

void process(std::string recv_data, SOCKET client_socket)
{
    cppNetworkUtil client;

    client.sendData(client.getHeaderText(client.header_parameters), client_socket);
    client.sendData("Hello World!", client_socket);
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