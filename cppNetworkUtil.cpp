#include "cppNetworkUtil.h"

std::string cppNetworkUtil::getHeaderMethod(const std::string buffer)
{
    std::string method;

    // 1. 查找第一个空格，它将 Method 与 URL 分开。
    size_t space_pos = buffer.find(' ');
    if (space_pos == std::string::npos)
    {
        // 如果没有找到空格，说明请求行格式不正确
        throw std::runtime_error("Invalid request line: No space found after method");
    }

    // 2. 提取 Method
    // 从字符串开头到第一个空格的位置就是 Method
    method = buffer.substr(0, space_pos);

    return method;
}

std::string cppNetworkUtil::getGetHeaderUrl(const std::string buffer)
{
    std::string url;

    // 1. 查找第一个空格，它将 Method 与 URL 分开。
    size_t frist_space_pos = buffer.find(' ');
    if (frist_space_pos == std::string::npos)
    {
        // 如果没有找到空格，说明请求行格式不正确
        throw std::runtime_error("Invalid request line: No space found after method");
    }

    // 2. 查找第二个空格，它将 URL 与 PROTOCOL 分开。
    size_t second_space_pos = buffer.find(' ', frist_space_pos);
    if (second_space_pos == std::string::npos)
    {
        // 如果没有找到空格，说明请求行格式不正确
        throw std::runtime_error("Invalid request line: No space found after url");
    }

    // 2. 提取 URL
    // 从字符串开头到第一个空格的位置就是 Method
    url = buffer.substr(frist_space_pos, second_space_pos);

    return url;
}

int cppNetworkUtil::getPostContentSize(const std::string buffer)
{
    // 查找Content-Length字段
    int pos = buffer.find("Content-Length:");
    if (pos == std::string::npos)
    {
        throw("Find Content-Length failed");
    }
    pos += 15; // 跳过"Content-Length:"
    if (buffer[pos] == ' ')
        pos++;

    // 提取Content-Length的值
    int contentLength = 0;
    size_t lengthStart = pos;
    while (pos < buffer.length() && buffer[pos] != '\r')
        pos++;
    std::string lengthStr = buffer.substr(lengthStart, pos - lengthStart);
    try
    {
        contentLength = stoi(lengthStr);
    }
    catch (const std::invalid_argument &)
    {
        throw("Parse Content-Length failed");
    }

    // 提取请求正文长度
    size_t bodyStart = buffer.find("\r\n\r\n", pos);
    if (bodyStart == std::string::npos)
    {
        throw("Find body failed");
    }
    bodyStart += 4; // 跳过"\r\n\r\n"

    return bodyStart + contentLength;
}

std::string cppNetworkUtil::getPostContentType(const std::string buffer)
{
    // 查找Content-Type字段
    int pos = buffer.find("Content-Type:");
    if (pos == std::string::npos)
    {
        throw("Find Content-Length failed");
    }
    pos += 13; // 跳过"Content-Type:"
    if (buffer[pos] == ' ')
        pos++;

    // 提取Content-Type的值
    size_t typeStart = pos;
    while (pos < buffer.length() && buffer[pos] != ';' && buffer[pos] != '\r' && buffer[pos] != '\n' && buffer[pos] != '\0')
        pos++;
    std::string contentType = buffer.substr(typeStart, pos - typeStart);

    return contentType;
}

std::string cppNetworkUtil::getPostContentBoundary(const std::string buffer)
{
    // 查找Boundary字段
    int pos = buffer.find("boundary=");
    if (pos == std::string::npos)
    {
        throw("Find boundary failed");
    }
    pos += 9; // 跳过"boundary="

    // 提取Boundary的值
    size_t boundaryStart = pos;
    while (pos < buffer.length() && buffer[pos] != '\r')
        pos++;
    std::string boundary = buffer.substr(boundaryStart, pos - boundaryStart);

    return boundary;
}

std::string cppNetworkUtil::getPostContentBody(const std::string buffer)
{
    // 查找Content-Length字段
    int pos = buffer.find("Content-Length:");
    if (pos == std::string::npos)
    {
        throw("Find Content-Length failed");
    }
    pos += 15; // 跳过"Content-Length:"
    if (buffer[pos] == ' ')
        pos++;

    // 提取Content-Length的值
    int contentLength = 0;
    size_t lengthStart = pos;
    while (pos < buffer.length() && buffer[pos] != '\r')
        pos++;
    std::string lengthStr = buffer.substr(lengthStart, pos - lengthStart);
    try
    {
        contentLength = std::stoi(lengthStr);
    }
    catch (const std::invalid_argument &)
    {
        throw("Parse Content-Length failed");
    }

    // 提取请求正文长度
    int bodyStart = buffer.find("\r\n\r\n", pos);
    if (bodyStart == std::string::npos)
    {
        throw("Find body failed");
        return "failed";
    }
    bodyStart += 4; // 跳过"\r\n\r\n"

    // 截取正文
    std::string body;
    if (bodyStart + contentLength <= buffer.length())
    {
        body = buffer.substr(bodyStart, contentLength);
    }
    else
    {
        throw("Size does not meet the requirements");
    }

    return body;
}

void cppNetworkUtil::sendData(const std::string data, SOCKET client_socket)
{
    send(client_socket, data.c_str(), data.size(), 0);
}

SOCKET cppNetworkUtil::start(int port)
{
    // WSA startup
#ifdef _WIN32
    WSADATA wsaData;
    int initResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (initResult != 0)
    {
        throw("WSAStartup failed");
    }
#endif

    // create socket
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET)
    {
#ifdef _WIN32
        WSACleanup();
#endif
        throw("Create socket failed");
    }

    // set SO_REUSEADDR options
    int optval = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval)) == -1)
    {
        closesocket(server_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        throw("Setsockopt failed");
    }

    // set server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;         // use IPv4
    server_address.sin_addr.s_addr = INADDR_ANY; // listen 0.0.0.0, all address
    server_address.sin_port = htons(port);       // set port

    // bind socket
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == SOCKET_ERROR)
    {
        closesocket(server_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        throw("Bind failed");
    }

    // listen for connections
    if (listen(server_socket, 1) == SOCKET_ERROR)
    {
        closesocket(server_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        throw("Listen failed");
    }

    return server_socket;
}

void *cppNetworkUtil::process(void (*func)(std::string recv_data, SOCKET client_socket), SOCKET client_socket)
{
    std::string recv_buffer;

    int recvd = 0;
    int totla_recvd = 0;
    int content_size = 0;

    char temp_buffer[BUFFERSIZE + 1]; // +1 是为了 null 终止符，如果直接作为 C 字符串打印的话
    memset(temp_buffer, 0, BUFFERSIZE);

    recvd = recv(client_socket, temp_buffer, BUFFERSIZE, 0);
    if (recvd > 0)
    {
        // 只附加实际接收到的字节数
        recv_buffer.append(temp_buffer, recvd);
        totla_recvd += recvd;
        // if (IS_DEBUG)
        //     std::cout << "recv " << recvd << " bytes, total recv: " << totla_recvd << " bytes\n";
    }
    else
    {
        // error
        int errorcode = WSAGetLastError();
        if (IS_DEBUG)
            printf("errorcode:%d\n", errorcode);

        return NULL;
    }

    std::string method;
    try
    {
        method = getHeaderMethod(recv_buffer);
    }
    catch (const std::exception &e)
    {
        if (IS_DEBUG)
            std::cerr << e.what() << '\n';
    }

    if (method == "POST")
    {
        content_size = getPostContentSize(recv_buffer);
        // if (IS_DEBUG)
        //     std::cout << "content_size=" << content_size << "\n";
        if (content_size == -1)
        {
            closesocket(client_socket);
            return NULL;
        }
    }

    // 循环接收数据
    while (totla_recvd < content_size)
    {
        char temp_buffer_in_do_while[BUFFERSIZE + 1]; // +1 是为了 null 终止符，如果您直接作为 C 字符串打印的话
        memset(temp_buffer_in_do_while, 0, BUFFERSIZE);
        recvd = recv(client_socket, temp_buffer_in_do_while, BUFFERSIZE, 0);

        if (IS_DEBUG)
            std::cout << "recvd=" << recvd << "\n";

        if (recvd > 0)
        {
            // 只附加实际接收到的字节数
            recv_buffer.append(temp_buffer_in_do_while, recvd);
            totla_recvd += recvd;
            // if (IS_DEBUG)
            // std::cout << "recv " << recvd << " bytes, total recv: " << totla_recvd << " bytes\n";
        }
        else if (recvd == 0)
        {
            // 客户端已优雅断开连接
            // if (IS_DEBUG)
            //     std::cout << "client close connect。" << "\n";
            break; // 跳出循环
        }
        else
        {
            // 发生error
            if (IS_DEBUG)
                std::cerr << "An error occurred while receiving, errno: " << errno << "\n";
            break; // 跳出循环
        }
    }

    // user function do something
    func(recv_buffer, client_socket);

    closesocket(client_socket);

    return NULL;
}

void cppNetworkUtil::exec(void (*func)(std::string recv_data, SOCKET client_socket), int thread_num, SOCKET server_socket)
{
    ThreadPool client_socket_thread_pool(thread_num);

    cppNetworkUtil obj;

    while (1)
    {
        SOCKET client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET)
        {
            if (IS_DEBUG)
                std::cerr << "accept failed: " << WSAGetLastError() << "\n";
            continue;
        }

        client_socket_thread_pool.enqueue(&cppNetworkUtil::process, &obj, func, client_socket);
    }
}

cppNetworkUtil::~cppNetworkUtil()
{
}