#include <iostream>

#include "cppNetworkUtil.h"

void process(std::string recv_data, SOCKET client_socket, SSL *ssl)
{
    if (ssl == nullptr)
    {
        log_e("SSL is not initialized, cannot process HTTPS request.\n");
        return;
    }

    cppNetworkUtil client;

    std::string header;
    std::string content;

    // https
    client.sendDataToHttpsHost(
        "www.example.com",
        "/",
        DEFAULT_SERVER_PORT,
        header,
        content,
        true); // enable CA verification(Optional)
    // http
    // client.sendDataToHttpHost(
    //     "www.example.com",
    //     "/",
    //     DEFAULT_SERVER_PORT,
    //     header,
    //     content);

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
        log_e("Error:%s\n", e.what());
    }
    catch (...)
    {
        log_e("Unknown error occurred.\n");
    }

    return 0;
}