#pragma once

#include <iostream>

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shlobj.h>

#pragma comment(lib, "ws2_32.lib")

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <ifaddrs.h>

typedef int SOCKET;

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#define WSAGetLastError() errno

#endif

#include <string>
#include <vector>
#include <thread>
#include <mutex>

#include "init.h"
#include "threadPool.h"

class cppNetworkUtil
{
public:
    /**
     * @brief Get the method in the GET request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Invalid request line: No space found after method
     *
     * @return method
     */
    std::string getHeaderMethod(const std::string buffer);

    /**
     * @brief Get the url in the GET request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Invalid request line: No space found after method
     * @throws Invalid request line: No space found after url
     *
     * @return url
     */
    std::string getGetHeaderUrl(const std::string buffer);

    /**
     * @brief Get the content size in the http POST request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Find Content-Length failed
     * @throws Parse Content-Length failed
     * @throws Find body failed
     *
     * @return content size
     */
    int getPostContentSize(const std::string buffer);

    /**
     * @brief Get the Content-Type in the http POST request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Find Content-Length failed
     *
     * @return Content-Type
     */
    std::string getPostContentType(const std::string buffer);

    /**
     * @brief Get the Boundary in the http POST request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Find Content-Length failed
     *
     * @return Boundary
     */
    std::string getPostContentBoundary(const std::string buffer);

    /**
     * @brief Get the content body in the http POST request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Find Content-Length failed
     *
     * @return content body
     */
    std::string getPostContentBody(const std::string buffer);

    /**
     * @brief Send data to the client
     *
     * @param data (const std::string) Data to be sent
     */
    void sendData(const std::string data, SOCKET client_socket);

    /**
     * @brief start server
     *
     * @param port (int) bind port
     *
     * @throws WSAStartup failed
     * @throws Create socket failed
     * @throws Setsockopt failed
     * @throws Bind failed
     * @throws Listen failed
     *
     * @return server_socket
     */
    SOCKET start(int port);

    /**
     * @brief exec server
     *
     * @param func (void (*func)(std::string recv_data, SOCKET client_socket)) action function
     * @param thread_num (int) thread num
     * @param server_socket (SOCKET) server socket
     */
    void exec(void (*func)(std::string recv_data, SOCKET client_socket), int thread_num, SOCKET server_socket);

    /**
     * @brief Automatically executed when leaving scope
     */
    ~cppNetworkUtil();

private:
    /**
     * @brief exec server
     *
     * @param func (void (*func)(std::string recv_data, SOCKET client_socket)) action function
     * @param server_socket (SOCKET) client socket
     */
    void *process(void (*func)(std::string recv_data, SOCKET client_socket), SOCKET client_socket);
};