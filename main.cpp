#include <iostream>

#include "cppNetworkUtil.h"

int port = DEFAULT_SERVER_PORT;

void process(std::string recv_data, SOCKET client_socket, SSL *ssl)
{
    if (ssl == nullptr)
    {
        if (IS_DEBUG)
            std::cerr << "SSL is not initialized, cannot process HTTPS request.\n";
        return;
    }

    cppNetworkUtil client;

    std::string header;
    std::string content;
    client.sendDataToHost(client.request_header_parameters.host, client.request_header_parameters.port, client.buildRequestHeader(client.request_header_parameters), header, content);

    client.response_header_parameters.mime_type = "text/html";

    client.sendDataToHttpsSocket(client.buildResponseHeader(client.response_header_parameters), ssl);
    client.sendDataToHttpsSocket(content, ssl);
}

int main(int argc, char **argv)
{
    cppNetworkUtil server;

    SOCKET server_socket;
    try
    {
        server.run(process);
    }
    catch (const std::exception &e)
    {
        if (IS_DEBUG)
        {
            std::cerr << e.what() << '\n';
        }
    }

    return 0;
}