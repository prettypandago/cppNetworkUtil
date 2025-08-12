#include <iostream>

#include "cppNetworkUtil.h"
#include "serverCallback.h"

#include "defines.h"
#include "log.h"

class MyServerHandler : public serverCallback
{
public:
    // Constructor receives a reference to cppNetworkUtil
    MyServerHandler(cppNetworkUtil &network_util) : network_util_(network_util) {}

    void onDataReceived(int client_id) override
    {
        cppNetworkUtil::clientConnectionInfo request_info = network_util_.getClientConnectionsInfo(client_id);

        std::map<std::string, std::string> parsed_header = network_util_.getParsedHeader(request_info.request_header);

        std::string header;  // received header
        std::string content; // received content
        std::map<std::string, std::string> response_header_parameters{{"status", "200"}, {"mime_type", "*/*"}, {"connection", "close"}};

        if (request_info.is_https_connection)
            response_header_parameters["enable_hsts"] = "true";

        if (parsed_header["method"] == "GET")
        {
            if (parsed_header["url"].empty())
            {
                try
                {
                    response_header_parameters["mime_type"] = "text/html";

#ifndef DISABLE_HTTPS
                    network_util_.sendDataToHttpsHost("www.example.com", "/", 443, header, content, true);
#else
                    network_util_.sendDataToHttpHost("www.example.com", "/", 80, header, content);
#endif
                }
                catch (const std::exception &e)
                {
                    log_e("Error: %s\n", e.what());
                }
                catch (...)
                {
                    log_e("Unknown error occurred while sending data.\n");
                }
            }
            else if (strcmp(parsed_header["url"].data(), "helloworld") == 0)
            {
                response_header_parameters["mime_type"] = "text/html";

                content = "<html><body><h1>Hello world</h1></body></html>";
            }
            else
            {
                response_header_parameters["status"] = "404";

                content = "<html><body><h1>404 Not Found!</h1></body></html>";
            }

            // send data to client
            if (request_info.is_https_connection)
            {
                network_util_.sendDataToHttpsSocket(client_id, network_util_.makeResponseHeader(response_header_parameters));
                network_util_.sendDataToHttpsSocket(client_id, content);
            }
            else
            {
                network_util_.sendDataToHttpSocket(client_id, network_util_.makeResponseHeader(response_header_parameters));
                network_util_.sendDataToHttpSocket(client_id, content);
            }
        }
    }

private:
    // Declare network_util_ as a member variable of the MyServerHandler class (otherwise there will be an error)
    cppNetworkUtil &network_util_;
};

int main(int argc, char **argv)
{
    cppNetworkUtil server;
    MyServerHandler handler(server);

    try
    {
        server.run(&handler, DEFAULT_HTTP_SERVER_PORT, DEFAULT_HTTPS_SERVER_PORT);
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