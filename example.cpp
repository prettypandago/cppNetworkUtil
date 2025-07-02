#include <iostream>

#include "cppNetworkUtil.h"
#include "serverCallback.h"

#include "defines.h"
#include "log.h"

class MyServerHandler : public serverCallback
{
public:
    // 构造函数接收 cppNetworkUtil 的引用
    MyServerHandler(cppNetworkUtil &network_util) : network_util_(network_util) {}

    void onDataReceived(int client_id) override
    {
        std::string recv_buffer = network_util_.get_client_connections_recv_buffer(client_id);

        std::string method;
        // Get method from the header
        try
        {
            method = network_util_.getHeaderMethod(recv_buffer);
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
            url = network_util_.getGetHeaderUrl(recv_buffer);
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
                // 发送响应数据到客户端
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
    // 声明 network_util_ 作为 MyServerHandler 类的成员变量 (不然会报错)
    cppNetworkUtil &network_util_;
};

int main(int argc, char **argv)
{
    cppNetworkUtil server;
    MyServerHandler handler(server); // 创建回调对象

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