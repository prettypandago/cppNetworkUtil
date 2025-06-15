#include "cppNetworkUtil.h"

// threadPool
// 构造函数实现（模板函数通常需要在头文件中定义）
inline ThreadPool::ThreadPool(size_t numThreads) : stop(false)
{
    if (numThreads == 0)
    {
        throw std::invalid_argument("Number of threads cannot be zero.");
    }
    for (size_t i = 0; i < numThreads; ++i)
    {
        workers.emplace_back([this]
                             {
			while (true)
			{
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(this->queueMutex);
					this->condition.wait(lock, [this]
					{
						return !this->tasks.empty() || this->stop;
					});

					if (this->stop && this->tasks.empty())
					{
						return;
					}
					task = std::move(this->tasks.front());
					this->tasks.pop();
				}
				task();
			} });
    }
}

// enqueue 模板函数的实现 (必须在头文件中)
template <class F, class... Args>
inline auto ThreadPool::enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();

    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (stop)
        {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        tasks.emplace([task]()
                      { (*task)(); });
    }
    condition.notify_one();
    return res;
}

// 析构函数实现
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}

// cppNetworkUtil
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

std::string cppNetworkUtil::getHeaderText(headerParameters parameter)
{
    std::string buffer;

    std::string title = "Unknown";
    if (parameter.status == 200)
        title = "OK";
    else if (parameter.status == 404)
        title = "Not Found";

    char *timebuf = new char[100];
    time_t now_date = time((time_t *)0);
    strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now_date));

    buffer += PROTOCOL;
    buffer += " ";
    buffer += std::to_string(parameter.status);
    buffer += " ";
    buffer += title;
    buffer += "\r\n";

    buffer += "Server: ";
    buffer += NAME;
    buffer += "\r\n";

    if (!parameter.mime_type.empty())
    {
        buffer += "Content-Type: ";
        buffer += parameter.mime_type;
        buffer += "\r\n";
    }

    if (!parameter.content_language.empty())
    {
        buffer += "Content-Language: ";
        buffer += parameter.content_language;
        buffer += "\r\n";
    }

    if (!parameter.cookie.empty())
    {
        buffer += "Set-Cookie: ";
        buffer += parameter.cookie;
        buffer += "\r\n";
    }

    buffer += "Connection: close\r\n";

    buffer += "\r\n";

    delete[] timebuf;

    return buffer;
}

// URL解码函数
std::string cppNetworkUtil::urlDecode(const std::string &encodedString)
{
    std::string decodedString;
    char hex[3];
    for (size_t i = 0; i < encodedString.length(); ++i)
    {
        if (encodedString[i] == '%')
        {
            if (i + 2 < encodedString.length())
            {
                hex[0] = encodedString[++i];
                hex[1] = encodedString[++i];
                hex[2] = '\0';
                decodedString += static_cast<char>(strtol(hex, nullptr, 16));
            }
            else
            {
                // 错误：无效的百分号编码
                decodedString += '%';
            }
        }
        else if (encodedString[i] == '+')
        {
            decodedString += ' ';
        }
        else
        {
            decodedString += encodedString[i];
        }
    }
    return decodedString;
}

// 解析url路径
std::vector<std::string> cppNetworkUtil::GetURLParameterRestfulapi(std::string url)
{
    std::vector<std::string> parts;

    // 如果路径以斜杠开头，则去除它
    if (!url.empty() && url[0] == '/')
    {
        url = url.substr(1);
    }

    std::stringstream ss(url);
    std::string segment;

    // 使用 getline 分隔符 '/' 读取
    while (getline(ss, segment, '/'))
    {
        if (!segment.empty()) // 确保不添加空字符串（例如，如果路径中有连续的斜杠）
        {
            parts.push_back(segment);
        }
    }

    return parts;
}

std::map<std::string, std::string> cppNetworkUtil::ParseUrlQueryParameters(const std::string &url)
{
    std::map<std::string, std::string> params;
    size_t question_pos = url.find('?');
    if (question_pos == std::string::npos || question_pos + 1 >= url.length())
        return params;

    std::string query = url.substr(question_pos + 1);
    std::stringstream ss(query);
    std::string pair;
    while (std::getline(ss, pair, '&'))
    {
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos)
        {
            std::string key = pair.substr(0, eq_pos);
            std::string value = pair.substr(eq_pos + 1);
            params[key] = cppNetworkUtil::urlDecode(value);
        }
        else if (!pair.empty())
        {
            params[pair] = "";
        }
    }
    return params;
}

// 解析 multipart 数据
std::vector<cppNetworkUtil::MultipartData> cppNetworkUtil::ParseMultipart(const std::string &boundary, const std::string &body)
{
    std::vector<cppNetworkUtil::MultipartData> parsedParts; // 存储所有解析出的部分
    std::string delimiter = "--" + boundary;                // 每个部分的开始分隔符
    std::string endDelimiter = delimiter + "--";            // 整个 multipart 结束的分隔符
    size_t pos = 0;                                         // 当前在 body 字符串中的查找位置

    // 跳过开头的空行，找到第一个有内容的位置
    pos = body.find_first_not_of("\r\n");
    if (pos == std::string::npos)
    {
        return parsedParts; // 如果 body 全是空行或为空，则返回空向量
    }

    // 循环查找每个数据部分
    while ((pos = body.find(delimiter, pos)) != std::string::npos)
    {
        pos += delimiter.length(); // 跳过当前分隔符

        // 检查是否是整个 multipart 数据的结束标记
        if (pos + 2 <= body.length() && body.substr(pos, 2) == "--")
        {
            break; // 找到结束标记，退出循环
        }

        // 找到下一个分隔符或结束标记的位置
        size_t nextPos = body.find(delimiter, pos);
        if (nextPos == std::string::npos)
        {
            nextPos = body.find(endDelimiter, pos);
            if (nextPos == std::string::npos)
            {
                break; // 既没有找到下一个分隔符，也没有找到结束分隔符，数据格式异常
            }
        }

        // 提取当前数据部分的原始字符串（包含头部和数据）
        std::string part = body.substr(pos, nextPos - pos);

        // 查找头部和数据之间的空行分隔符 (CRLFCRLF 或 LFLF)
        size_t headersEnd = part.find("\r\n\r\n");
        if (headersEnd == std::string::npos)
        {
            headersEnd = part.find("\n\n"); // 尝试 Unix 风格换行符
        }
        if (headersEnd == std::string::npos)
        {
            continue; // 如果没有找到头部和数据的分隔符，则跳过此部分
        }

        std::string headers = part.substr(0, headersEnd);
        std::string headers_lower = headers;
        transform(headers_lower.begin(), headers_lower.end(), headers_lower.begin(), ::toupper);           // 提取头部字符串
        std::string data = part.substr(headersEnd + (part.find("\r\n\r\n") != std::string::npos ? 4 : 2)); // 提取数据字符串，跳过分隔符长度

        MultipartData currentPart; // 创建一个新的 ParsedPart 对象来存储当前部分的信息

        // --- 提取 name 属性 ---
        size_t namePos = headers.find("name=\"");
        if (namePos != std::string::npos)
        {
            namePos += 6; // 跳过 "name=\"" 的长度
            size_t nameEnd = headers.find("\"", namePos);
            if (nameEnd != std::string::npos)
            {
                currentPart.name = headers.substr(namePos, nameEnd - namePos);
            }
        }

        // --- 提取 filename 属性 ---
        size_t filenamePos = headers.find("filename=\"");
        if (filenamePos != std::string::npos)
        {
            filenamePos += 10; // 跳过 "filename=\"" 的长度
            size_t filenameEnd = headers.find("\"", filenamePos);
            if (filenameEnd != std::string::npos)
            {
                currentPart.filename = headers.substr(filenamePos, filenameEnd - filenamePos);
            }
        }

        // --- 提取 Content-Type 属性 ---
        // 不区分大小写查找
        size_t contentTypePos = headers_lower.find("content-type:");
        if (contentTypePos != std::string::npos)
        {
            contentTypePos += 13; // 跳过 "content-type:" 的长度
            size_t contentTypeEnd = headers_lower.find("\r\n", contentTypePos);
            if (contentTypeEnd == std::string::npos)
            {
                contentTypeEnd = headers_lower.find("\n", contentTypePos); // 尝试 Unix 风格换行符
            }

            if (contentTypeEnd != std::string::npos)
            {
                // 提取 Content-Type 值，并去除前后的空白字符
                std::string typeStr = headers_lower.substr(contentTypePos, contentTypeEnd - contentTypePos);
                size_t firstChar = typeStr.find_first_not_of(" \t");
                if (firstChar != std::string::npos)
                {
                    size_t lastChar = typeStr.find_last_not_of(" \t");
                    currentPart.content_type = typeStr.substr(firstChar, lastChar - firstChar + 1);
                }
            }
        }

        // --- 检查是否有 Content-Length 头并根据其截取数据 ---
        // Content-Length 并不常用在 multipart 的单个部分中，但如果存在，则遵守它
        size_t contentLengthPos = headers.find("Content-Length:");
        int contentLength = -1; // 默认值为 -1 表示未知或无效
        if (contentLengthPos != std::string::npos)
        {
            contentLengthPos += 15; // 跳过 "Content-Length: " 的长度
            size_t lengthEnd = headers.find("\r\n", contentLengthPos);
            if (lengthEnd == std::string::npos)
            {
                lengthEnd = headers.find("\n", contentLengthPos);
            }

            if (lengthEnd != std::string::npos)
            {
                std::string lengthStr = headers.substr(contentLengthPos, lengthEnd - contentLengthPos);
                try
                {
                    contentLength = std::stoi(lengthStr); // 尝试将字符串转换为整数
                }
                catch (...)
                {
                    contentLength = -1; // 转换失败，视为无效长度
                }
            }
        }

        // 根据解析到的 Content-Length 来截取数据
        if (contentLength >= 0 && contentLength < data.length())
        {
            currentPart.data = data.substr(0, contentLength);
        }
        else
        {
            // 如果没有 Content-Length 或其值无效，尝试修剪数据尾部的空白字符
            size_t lastNonSpace = data.find_last_not_of(" \t\r\n");
            if (lastNonSpace != std::string::npos)
            {
                currentPart.data = data.substr(0, lastNonSpace + 1);
            }
            else
            {
                currentPart.data = ""; // 如果数据全是空白，则数据为空
            }
        }

        parsedParts.push_back(currentPart); // 将解析出的当前部分添加到结果向量中
        pos = nextPos;                      // 更新查找位置，继续查找下一个分隔符
    }

    return parsedParts; // 返回所有解析出的部分
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