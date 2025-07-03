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
        std::string request_data = network_util_.get_client_connections_request_data(client_id);
        std::string request_header = network_util_.get_client_connections_request_header(client_id);
        std::string request_content = network_util_.get_client_connections_request_content(client_id);

        std::string method;
        // Get method from the header
        try
        {
            method = network_util_.getHeaderMethod(request_header);
        }
        catch (const std::exception &e)
        {
            log_e("Error getting header method: %s\n", e.what());
        }
        catch (...)
        {
            log_e("Unknown error occurred while getting header method.\n");
        }

        std::string url;
        // Get url from the header
        try
        {
            url = network_util_.getGetHeaderUrl(request_header);
        }
        catch (const std::exception &e)
        {
            log_e("Error getting GET header URL: %s\n", e.what());
        }
        catch (...)
        {
            log_e("Unknown error occurred while getting GET header URL.\n");
        }

        std::string header;  // received header
        std::string content; // received content
        responseHeaderParameters response_header_parameters;

        if (method == "GET")
        {
            if (url.empty())
            {
                try
                {
                    response_header_parameters.mime_type = "text/html";

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
            else if (strcmp(url.c_str(), "helloworld") == 0)
            {
                response_header_parameters.mime_type = "text/html";

                content = "<html><body><h1>Hello world</h1></body></html>";
            }
            else
            {
                response_header_parameters.status = 404;
                content = "<html><body><h1>404 Not Found!</h1></body></html>";
            }

// send data to client
#ifndef DISABLE_HTTPS
            network_util_.sendDataToHttpsSocket(client_id, network_util_.buildResponseHeader(response_header_parameters));
            network_util_.sendDataToHttpsSocket(client_id, content);
#else
            network_util_.sendDataToHttpSocket(client_id, network_util_.buildResponseHeader(response_header_parameters));
            network_util_.sendDataToHttpSocket(client_id, content);
#endif
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
        server.run(DEFAULT_SERVER_PORT, &handler);
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