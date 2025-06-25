#include <iostream>

#include "cppNetworkUtil.h"

int port = DEFAULT_SERVER_PORT;

void process(std::string recv_data, SOCKET client_socket)
{
    cppNetworkUtil client;

    std::string header;
    std::string content;
    client.sendDataToHost(client.request_header_parameters.host, client.request_header_parameters.port, client.buildRequestHeader(client.request_header_parameters), header, content);

    client.response_header_parameters.mime_type = "text/html";

    client.sendDataToClient(client.buildResponseHeader(client.response_header_parameters), client_socket);
    client.sendDataToClient(content, client_socket);
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