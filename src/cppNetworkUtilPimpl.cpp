#include "cppNetworkUtilPimpl.h"
#include "serverCallback.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

cppNetworkUtilPimpl::cppNetworkUtilPimpl() : ssl_ctx_server(nullptr), ssl_ctx_client(nullptr), ssl(nullptr)
{
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    ssl_ctx_server = SSL_CTX_new(TLS_server_method());
    ssl_ctx_client = SSL_CTX_new(TLS_client_method());
}

cppNetworkUtilPimpl::~cppNetworkUtilPimpl()
{
    if (ssl)
    {
        SSL_free(ssl);
    }
    if (ssl_ctx_server)
    {
        SSL_CTX_free(ssl_ctx_server);
    }
    if (ssl_ctx_client)
    {
        SSL_CTX_free(ssl_ctx_client);
    }
    EVP_cleanup(); // 清理 OpenSSL 资源
}

std::map<std::string, std::string> cppNetworkUtilPimpl::getParsedHeader_Pimpl(const std::string &header)
{
    std::map<std::string, std::string> parsed_header;

    // 查找第一个空格，它将 Method 与 URL 分开。
    size_t frist_space_pos = header.find(' ');
    if (frist_space_pos == std::string::npos)
    {
        // 如果没有找到空格，说明请求行格式不正确
        throw std::runtime_error("Invalid request line: No space found after method");
    }

    // 提取 Method
    // 从字符串开头到第一个空格的位置就是 Method
    parsed_header["method"] = header.substr(0, frist_space_pos);

    // 查找第二个空格，它将 URL 与 PROTOCOL 分开。
    size_t second_space_pos = header.find(' ', frist_space_pos + 1);
    if (second_space_pos == std::string::npos)
    {
        // 如果没有找到空格，说明请求行格式不正确
        throw std::runtime_error("Invalid request line: No space found after url");
    }

    // 取 URL
    // 从字符串第一个空格到第二个空格的位置就是 URL
    // 注意: substr的第二个参数是长度, 在这里踩坑了
    parsed_header["url"] = header.substr(frist_space_pos + 1, second_space_pos - frist_space_pos - 1);

    if (parsed_header["url"][0] == '/')
    {
        parsed_header["url"].erase(0, 1); // 去除'/'
    }

    // 提取 HTTP 版本
    // 从第二个空格到行尾（或\r\n）为 HTTP 版本
    size_t line_end_pos = header.find("\r\n", second_space_pos + 1);
    if (line_end_pos == std::string::npos)
    {
        throw std::runtime_error("Invalid request line: No \r\n found after http version");
    }
    parsed_header["http_version"] = header.substr(second_space_pos + 1, line_end_pos - second_space_pos - 1);

    // 解析剩余的请求头字段
    size_t headers_start = header.find("\r\n");
    if (headers_start != std::string::npos)
    {
        headers_start += 2; // 跳过请求行后的 "\r\n"
        size_t headers_end = header.find("\r\n\r\n", headers_start);
        if (headers_end == std::string::npos)
            headers_end = header.length();

        std::string headers_section = header.substr(headers_start, headers_end - headers_start);
        std::istringstream stream(headers_section);
        std::string line;
        while (std::getline(stream, line))
        {
            // 移除行尾的 '\r'
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos)
            {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                // 去除 value 前后的空白
                size_t first = value.find_first_not_of(" \t");
                size_t last = value.find_last_not_of(" \t");
                if (first != std::string::npos && last != std::string::npos)
                    value = value.substr(first, last - first + 1);
                else
                    value = "";
                parsed_header[key] = value;
            }
        }
    }

    return parsed_header;
}

int cppNetworkUtilPimpl::getContentSize_Pimpl(const std::string buffer)
{
    // 查找Content-Length字段
    size_t pos = buffer.find("Content-Length:");
    if (pos == std::string::npos)
    {
        throw("Find Content-Length failed");
    }
    pos += 15; // 跳过"Content-Length:"
    if (buffer[pos] == ' ')
        pos++;

    // 提取Content-Length的值
    size_t contentLength = 0;
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
    catch (...)
    {
        throw("Parse Content-Length failed with unknown error");
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

std::string cppNetworkUtilPimpl::getHeaderValue_Pimpl(const std::string &headers, const std::string &key)
{
    std::string searchKey = key + ":";
    size_t pos = headers.find(searchKey);
    if (pos != std::string::npos)
    {
        size_t start = pos + searchKey.length();
        size_t end = headers.find("\r\n", start);
        if (end != std::string::npos)
        {
            std::string value = headers.substr(start, end - start);
            // 裁剪前导/尾随空白
            size_t first = value.find_first_not_of(" \t");
            if (first == std::string::npos)
                return ""; // 全是空白
            size_t last = value.find_last_not_of(" \t");
            return value.substr(first, (last - first + 1));
        }
    }
    return "";
}

int cppNetworkUtilPimpl::getPostContentSize_Pimpl(const std::string &buffer)
{
    // 查找Content-Length字段
    int pos = buffer.find("Content-Length:");
    if (pos == std::string::npos)
    {
        throw std::runtime_error("Find Content-Length failed");
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
        throw std::runtime_error("Parse Content-Length failed");
    }

    // 提取请求正文长度
    size_t bodyStart = buffer.find("\r\n\r\n", pos);
    if (bodyStart == std::string::npos)
    {
        throw std::runtime_error("Find body failed");
    }
    bodyStart += 4; // 跳过"\r\n\r\n"

    return bodyStart + contentLength;
}

std::string cppNetworkUtilPimpl::getPostContentType_Pimpl(const std::string &buffer)
{
    // 查找Content-Type字段
    int pos = buffer.find("Content-Type:");
    if (pos == std::string::npos)
    {
        log_e("Find Content-Type failed!\n");
        return "failed";
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

std::string cppNetworkUtilPimpl::getPostContentBoundary_Pimpl(const std::string &buffer)
{
    // 查找Boundary字段
    int pos = buffer.find("boundary=");
    if (pos == std::string::npos)
    {
        log_e("Find boundary failed!\n");
        return "failed";
    }
    pos += 9; // 跳过"boundary="

    // 提取Boundary的值
    size_t boundaryStart = pos;
    while (pos < buffer.length() && buffer[pos] != '\r')
        pos++;
    std::string boundary = buffer.substr(boundaryStart, pos - boundaryStart);

    return boundary;
}

std::string cppNetworkUtilPimpl::getPostContentBody_Pimpl(const std::string buffer)
{
    // 查找Content-Length字段
    size_t pos = buffer.find("Content-Length:");
    if (pos == std::string::npos)
    {
        throw("Find Content-Length failed");
    }
    pos += 15; // 跳过"Content-Length:"
    if (buffer[pos] == ' ')
        pos++;

    // 提取Content-Length的值
    size_t contentLength = 0;
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
    catch (...)
    {
        throw("Parse Content-Length failed with unknown error");
    }

    // 提取请求正文长度
    size_t bodyStart = buffer.find("\r\n\r\n", pos);
    if (bodyStart == std::string::npos)
    {
        throw("Find body failed");
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

std::string cppNetworkUtilPimpl::makeResponseHeader_Pimpl(std::map<std::string, std::string> parameters)
{
    std::string buffer;

    if (parameters.count("status") && parameters.count("connection"))
    {
        std::string title;
        auto it = http_code.find(std::stoi(parameters["status"]));
        if (it != http_code.end())
            title = it->second;
        else
            title = "Unknown";

        buffer += PROTOCOL;
        buffer += " ";
        buffer += parameters["status"];
        buffer += " ";
        buffer += title;
        buffer += "\r\n";

        buffer += "Connection: ";
        buffer += parameters["connection"];
        buffer += "\r\n";
    }
    else
    {
        throw std::runtime_error("Missing required fields");
    }

    char *timebuf = new char[100];
    time_t now_date = time((time_t *)0);
    strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now_date));

    buffer += "Server: ";
    buffer += NAME;
    buffer += "\r\n";

    if (parameters["enable_hsts"] == "true")
        buffer += "Strict-Transport-Security: max-age=31536000; includeSubDomains; preload\r\n";

    for (const auto &pair : parameters)
    {
        if (pair.first != "status" && pair.first != "connection" && pair.first != "enable_hsts")
            buffer += pair.first + ": " + pair.second + "\r\n";
    }
    buffer += "\r\n";

    delete[] timebuf;

    return buffer;
}

std::string cppNetworkUtilPimpl::makeRequestHeader_Pimpl(std::map<std::string, std::string> parameters)
{
    std::string buffer;

    if (parameters.count("method") && parameters.count("path") && parameters.count("host"))
    {
        buffer += parameters["method"] + " " + parameters["path"] + " " + PROTOCOL + "\r\n";
        buffer += "Host: " + parameters["host"];
        if (parameters.count("port") && parameters["port"] != "80" && parameters["port"] != "443")
        {
            buffer += ":" + parameters["port"];
        }
        buffer += "\r\n";
    }
    else
    {
        throw std::runtime_error("Missing required fields");
    }

    if (parameters.count("connection"))
    {
        buffer += "Connection: " + parameters["connection"] + "\r\n";
    }
    else
    {
        buffer += "Connection: close\r\n";
    }

    for (const auto &kv : parameters)
    {
        if (kv.first == "method" || kv.first == "path" || kv.first == "host" || kv.first == "port" || kv.first == "connection")
            continue;
        buffer += kv.first + ": " + kv.second + "\r\n";
    }

    buffer += "\r\n";

    return buffer;
}

// URL解码函数
std::string cppNetworkUtilPimpl::urlDecode_Pimpl(const std::string &encodedString)
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

std::map<std::string, std::string> cppNetworkUtilPimpl::parseUrlEncodedFormBody_Pimpl(const std::string &postBody)
{
    std::map<std::string, std::string> formData;

    std::string data = urlDecode_Pimpl(postBody);

    if (data.empty())
    {
        return formData; // 空消息体直接返回空map
    }

    std::string::size_type prevPos = 0;
    std::string::size_type pos = data.find('&', prevPos);

    while (pos != std::string::npos)
    {
        std::string pair = data.substr(prevPos, pos - prevPos);
        size_t eqPos = pair.find('=');
        if (eqPos != std::string::npos)
        {
            std::string key = pair.substr(0, eqPos);
            std::string value = pair.substr(eqPos + 1);
            formData[key] = value;
        }
        prevPos = pos + 1; // 跳过'&'
        pos = data.find('&', prevPos);
    }

    // 处理最后一个键值对（如果没有以&结尾）
    std::string lastPair = data.substr(prevPos);
    if (!lastPair.empty())
    {
        size_t eqPos = lastPair.find('=');
        if (eqPos != std::string::npos)
        {
            std::string key = lastPair.substr(0, eqPos);
            std::string value = lastPair.substr(eqPos + 1);
            formData[key] = value;
        }
    }

    return formData;
}

// 解析url路径
std::vector<std::string> cppNetworkUtilPimpl::getURLParameterRestfulapi_Pimpl(std::string url)
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

std::map<std::string, std::string> cppNetworkUtilPimpl::parseUrlQueryParameters_Pimpl(const std::string &url)
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
            params[key] = urlDecode_Pimpl(value);
        }
        else if (!pair.empty())
        {
            params[pair] = "";
        }
    }
    return params;
}

// 解析 multipart 数据
std::map<std::string, multipartData> cppNetworkUtilPimpl::parseMultipart_Pimpl(const std::string &boundary, const std::string &body)
{
    std::map<std::string, multipartData> parsedParts; // 存储所有解析出的部分
    std::string delimiter = "--" + boundary;          // 每个部分的开始分隔符
    std::string endDelimiter = delimiter + "--";      // 整个 multipart 结束的分隔符
    size_t pos = 0;                                   // 当前在 body 字符串中的查找位置

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

        std::string name;

        // --- 提取 name 属性 ---
        size_t namePos = headers.find("name=\"");
        if (namePos != std::string::npos)
        {
            namePos += 6; // 跳过 "name=\"" 的长度
            size_t nameEnd = headers.find("\"", namePos);
            if (nameEnd != std::string::npos)
            {
                name = headers.substr(namePos, nameEnd - namePos);
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
                parsedParts[name].filename = headers.substr(filenamePos, filenameEnd - filenamePos);
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
                    parsedParts[name].content_type = typeStr.substr(firstChar, lastChar - firstChar + 1);
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
            parsedParts[name].data = data.substr(0, contentLength);
        }
        else
        {
            // 如果没有 Content-Length 或其值无效，尝试修剪数据尾部的空白字符
            size_t lastNonSpace = data.find_last_not_of(" \t\r\n");
            if (lastNonSpace != std::string::npos)
            {
                parsedParts[name].data = data.substr(0, lastNonSpace + 1);
            }
            else
            {
                parsedParts[name].data = ""; // 如果数据全是空白，则数据为空
            }
        }

        pos = nextPos; // 更新查找位置，继续查找下一个分隔符
    }

    return parsedParts; // 返回所有解析出的部分
}

void cppNetworkUtilPimpl::sendDataToHttpSocket_Pimpl(SOCKET socket, const std::string &data)
{
    send(socket, data.data(), data.length(), 0); // 发送数据到套接字
}

void cppNetworkUtilPimpl::sendDataToHttpsSocket_Pimpl(SOCKET socket, const std::string &data)
{
    SSL_write(client_connections[socket].ssl, data.data(), data.length()); // 发送数据到 SSL 套接字
}

void cppNetworkUtilPimpl::sendDataToHttpHost_Pimpl(const std::string &host, const std::string &path, int port, std::string &header, std::string &content)
{
#ifdef _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        throw std::runtime_error("WSAStartup failed");
    }
#endif

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        HANDLE_ERROR("Failed to create socket");
#ifdef _WIN32
        WSACleanup();
#endif
        throw std::runtime_error("Failed to create socket");
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    struct hostent *server_host = gethostbyname(host.data());
    if (server_host == NULL)
    {
        log_e("Failed to resolve host: %s\n", host.data());
        closesocket(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        throw std::runtime_error("Failed to resolve host");
    }
    memcpy(&server_addr.sin_addr, server_host->h_addr_list[0], server_host->h_length);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        HANDLE_ERROR("Failed to connect to server");
        closesocket(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        throw std::runtime_error("Failed to connect to server");
    }

    std::string request_header_str = makeRequestHeader_Pimpl({{"method", "GET"}, {"host", host}, {"path", path}, {"port", std::to_string(port)}, {"connection", "close"}});
    int bytes_sent = send(sock, request_header_str.data(), request_header_str.length(), 0);
    if (bytes_sent == SOCKET_ERROR)
    {
        HANDLE_ERROR("Failed to send request");
        closesocket(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        throw std::runtime_error("Failed to send request");
    }

    // 接收响应头部
    std::string response_buffer;
    char buffer[BUFFERSIZE + 1];
    size_t header_end_pos = std::string::npos;
    bool header_found = false;

    while (!header_found)
    {
        memset(buffer, '\0', sizeof(buffer));
        int bytes_read = recv(sock, buffer, BUFFERSIZE, 0);
        if (bytes_read <= 0)
        {
            HANDLE_ERROR("Failed to read from socket during header reception or connection closed");
            closesocket(sock);
#ifdef _WIN32
            WSACleanup();
#endif
            throw std::runtime_error("Connection closed before complete header reception or socket read error");
        }
        response_buffer.append(buffer, bytes_read);
        header_end_pos = response_buffer.find("\r\n\r\n");
        if (header_end_pos != std::string::npos)
        {
            header_found = true;
        }
    }

    header = response_buffer.substr(0, header_end_pos);
    std::string remaining_buffer_after_header = response_buffer.substr(header_end_pos + 4);

    // 检查 Transfer-Encoding 头部
    std::string transfer_encoding = getHeaderValue_Pimpl(header, "Transfer-Encoding");
    std::transform(transfer_encoding.begin(), transfer_encoding.end(), transfer_encoding.begin(), ::tolower);

    if (transfer_encoding == "chunked")
    {
        // --- 直接在函数内部处理分块传输编码 ---
        std::string current_chunk_buffer = remaining_buffer_after_header; // 从初始缓冲区中剩余的数据开始
        content.clear();                                                  // 清空内容，准备接收解码后的数据

        while (true)
        {
            size_t line_end_pos = current_chunk_buffer.find("\r\n");
            while (line_end_pos == std::string::npos)
            {
                // 缓冲区中没有完整的行，需要从套接字中读取更多数据
                char temp_buffer[BUFFERSIZE + 1];
                memset(temp_buffer, '\0', sizeof(temp_buffer));
                int bytes_read_chunk = recv(sock, temp_buffer, BUFFERSIZE, 0);
                if (bytes_read_chunk <= 0)
                {
                    HANDLE_ERROR("Failed to read from socket during chunk size reception");
                    closesocket(sock);
#ifdef _WIN32
                    WSACleanup();
#endif
                    throw std::runtime_error("Socket read failed during chunk size reception");
                }
                current_chunk_buffer.append(temp_buffer, bytes_read_chunk);
                line_end_pos = current_chunk_buffer.find("\r\n");
            }

            if (line_end_pos == std::string::npos)
            {          // 仍然没有完整行，可能连接已关闭或数据异常
                break; // 退出外层while循环
            }

            std::string chunk_size_str = current_chunk_buffer.substr(0, line_end_pos);
            // 移除分块大小后的扩展信息（如 ";chunk-extension"），只保留十六进制大小
            size_t semi_colon_pos = chunk_size_str.find(';');
            if (semi_colon_pos != std::string::npos)
            {
                chunk_size_str = chunk_size_str.substr(0, semi_colon_pos);
            }

            long chunk_size;
            try
            {
                chunk_size = std::stoul(chunk_size_str, nullptr, 16); // 将十六进制字符串转换为数字
            }
            catch (const std::exception &e)
            {
                log_e("Failed to parse chunk size '%s': {%s}\n", chunk_size_str.data(), e.what());
                closesocket(sock);
#ifdef _WIN32
                WSACleanup();
#endif
                throw std::runtime_error("Failed to parse chunk size");
            }
            catch (...)
            {
                log_e("Failed to parse chunk size '%s' with unknown error\n", chunk_size_str.data());
                closesocket(sock);
#ifdef _WIN32
                WSACleanup();
#endif
                throw std::runtime_error("Failed to parse chunk size with unknown error");
            }

            // 移除已解析的分块大小行（包括 CRLF）
            current_chunk_buffer = current_chunk_buffer.substr(line_end_pos + 2);

            if (chunk_size == 0)
            {
                // 遇到最后一个空块，表示分块数据结束
                // 还需要读取最后的 CRLF
                if (current_chunk_buffer.length() < 2)
                {
                    char temp_crlf[2];
                    recv(sock, temp_crlf, 2, 0); // 读取最后的 CRLF
                }
                else
                {
                    current_chunk_buffer = current_chunk_buffer.substr(2); // 移除最后的 CRLF
                }
                break; // 退出外层while循环
            }

            // 读取块数据
            long bytes_needed_for_chunk = chunk_size;
            while (bytes_needed_for_chunk > 0)
            {
                if (!current_chunk_buffer.empty())
                {
                    long bytes_from_buffer = std::min((long)current_chunk_buffer.length(), bytes_needed_for_chunk);
                    content.append(current_chunk_buffer.data(), bytes_from_buffer);
                    current_chunk_buffer = current_chunk_buffer.substr(bytes_from_buffer);
                    bytes_needed_for_chunk -= bytes_from_buffer;
                }
                else
                {
                    char data_buffer[BUFFERSIZE + 1];
                    memset(data_buffer, '\0', sizeof(data_buffer));
                    int bytes_read_data = recv(sock, data_buffer, std::min((long)BUFFERSIZE, bytes_needed_for_chunk), 0);
                    if (bytes_read_data <= 0)
                    {
                        HANDLE_ERROR("Failed to read from socket during chunk data reception");
                        closesocket(sock);
#ifdef _WIN32
                        WSACleanup();
#endif
                        throw std::runtime_error("Incomplete chunk data: connection closed unexpectedly or socket read error");
                    }
                    content.append(data_buffer, bytes_read_data);
                    bytes_needed_for_chunk -= bytes_read_data;
                }
            }

            // 读取每个块数据后的 CRLF
            if (current_chunk_buffer.length() < 2)
            {
                char temp_crlf[2];
                recv(sock, temp_crlf, 2, 0); // 读取 CRLF
            }
            else
            {
                current_chunk_buffer = current_chunk_buffer.substr(2); // 移除 CRLF
            }
        }
    }
    else
    {
        // 非分块编码，尝试通过 Content-Length 或直接读取直到连接关闭
        std::string content_length_str = getHeaderValue_Pimpl(header, "Content-Length");
        long content_length = -1;
        if (!content_length_str.empty())
        {
            try
            {
                content_length = std::stol(content_length_str);
            }
            catch (const std::exception &e)
            {
                log_e("Failed to parse Content-Length: {%s}\n", e.what());
                // 继续尝试读取直到连接关闭
            }
            catch (...)
            {
                log_e("Failed to parse Content-Length with unknown error\n");
                // 继续尝试读取直到连接关闭
            }
        }

        content = remaining_buffer_after_header; // 将头部后面的剩余数据作为内容的开始

        long bytes_read_body = content.length();
        while (true)
        {
            if (content_length != -1 && bytes_read_body >= content_length)
            {
                break; // 已读取指定长度的内容
            }

            memset(buffer, '\0', sizeof(buffer));
            int bytes_from_sock = recv(sock, buffer, BUFFERSIZE, 0);
            if (bytes_from_sock <= 0)
            {
                if (bytes_from_sock == 0)
                { // Connection closed normally
                    break;
                }
                HANDLE_ERROR("Failed to read from socket during body reception");
                break; // 连接关闭或错误
            }
            content.append(buffer, bytes_from_sock);
            bytes_read_body += bytes_from_sock;
        }
    }

    closesocket(sock);
#ifdef _WIN32
    WSACleanup();
#endif
}

void cppNetworkUtilPimpl::sendDataToHttpsHost_Pimpl(const std::string &host, const std::string &path, int port, std::string &header, std::string &content, bool enable_CA)
{
#ifdef _WIN32
    // Windows Sockets 初始化
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        throw std::runtime_error("WSAStartup failed");
    }
#endif

    // 加载系统信任的CA证书，用于验证服务器证书。
    if (enable_CA)
    {
        if (SSL_CTX_set_default_verify_paths(ssl_ctx_client) != 1)
        {
            HANDLE_ERROR("Failed to set default verify paths for client context");
            throw std::runtime_error("Failed to load default CA certificates");
        }
    }

    // 创建SSL对象
    SSL *ssl_conn = SSL_new(ssl_ctx_client);
    if (ssl_conn == nullptr)
    {
        HANDLE_ERROR("SSL creation failed");
        throw std::runtime_error("Failed to create SSL object");
    }

    // 创建BIO连接
    BIO *bio = BIO_new_connect((char *)(std::string(host) + ":" + std::to_string(port)).data());
    if (bio == nullptr)
    {
        SSL_free(ssl_conn);
        HANDLE_ERROR("BIO creation failed");
        throw std::runtime_error("Failed to create BIO connection");
    }

    // 设置连接BIO到SSL对象
    SSL_set_bio(ssl_conn, bio, bio); // SSL_set_bio 会接管bio的所有权

    // 执行TLS/SSL握手
    // BIO_do_connect() 会尝试建立底层TCP连接
    if (BIO_do_connect(bio) <= 0)
    {
        HANDLE_ERROR("BIO connection failed");

        // Clean up resources before returning
        if (ssl_conn)
        {
            SSL_free(ssl_conn); // This also frees the associated BIO
        }
#ifdef _WIN32
        WSACleanup();
#endif
        throw std::runtime_error("Failed to connect to server");
    }

    // 执行 SSL/TLS 握手
    if (SSL_connect(ssl_conn) <= 0)
    {
        // 握手失败，打印 OpenSSL 错误信息
        HANDLE_ERROR("SSL handshake failed");
        SSL_free(ssl_conn); // 释放 SSL 结构
#ifdef _WIN32
        WSACleanup();
#endif
        throw std::runtime_error("SSL handshake failed");
    }

    // 验证服务器证书
    if (enable_CA)
    {
        X509 *cert = SSL_get_peer_certificate(ssl_conn);
        if (cert)
        {
            // std::cout << "Server certificate found." << std::endl;
            long verify_result = SSL_get_verify_result(ssl_conn);
            // if (verify_result == X509_V_OK)
            // {
            //     std::cout << "Server certificate verified successfully." << std::endl;
            // }
            // else
            // {
            //     std::cerr << "Warning: Server certificate verification failed: "
            //               << X509_verify_cert_error_string(verify_result) << std::endl;
            // }
            X509_free(cert); // 释放证书对象
        }
        else
        {
            // std::cerr << "Warning: No server certificate presented." << std::endl;
        }
    }

    // 发送HTTPS请求 (HTTP协议部分)
    std::string request_header = makeRequestHeader_Pimpl({{"method", "GET"}, {"host", host}, {"path", path}, {"port", std::to_string(port)}, {"connection", "close"}});

    int bytes_written = SSL_write(ssl_conn, request_header.data(), request_header.length());
    // std::cout << "Sent " << bytes_written << " bytes request." << std::endl;

    // 接收响应
    std::string response_buffer = "";
    char buffer[BUFFERSIZE + 1]; // +1 for null terminator
    int total_bytes_read = 0;    // 用于跟踪已读取的总字节数
    size_t header_end_pos = std::string::npos;
    bool header_found = false;

    while (!header_found)
    {
        memset(buffer, '\0', sizeof(buffer));
        int bytes_read = SSL_read(ssl_conn, buffer, BUFFERSIZE);
        if (bytes_read <= 0)
        {
            int err = SSL_get_error(ssl_conn, bytes_read);
            if (err != SSL_ERROR_ZERO_RETURN)
            {
                HANDLE_ERROR("SSL read failed during header reception");
            }
            SSL_free(ssl_conn);
            throw std::runtime_error("Connection closed before complete header reception or SSL read error");
        }
        response_buffer.append(buffer, bytes_read);
        total_bytes_read += bytes_read;
        header_end_pos = response_buffer.find("\r\n\r\n");
        if (header_end_pos != std::string::npos)
        {
            header_found = true;
        }
    }

    header = response_buffer.substr(0, header_end_pos);
    std::string remaining_buffer_after_header = response_buffer.substr(header_end_pos + 4); // +4 to skip the "\r\n\r\n"

    // 检查 Transfer-Encoding 头部
    std::string transfer_encoding = getHeaderValue_Pimpl(header, "Transfer-Encoding");
    std::transform(transfer_encoding.begin(), transfer_encoding.end(), transfer_encoding.begin(), ::tolower);

    if (transfer_encoding == "chunked")
    {
        // 需要分块接收
        std::string current_chunk_buffer = remaining_buffer_after_header; // 从初始缓冲区中剩余的数据开始
        content.clear();                                                  // 清空内容，准备接收解码后的数据

        while (true)
        {
            size_t line_end_pos = current_chunk_buffer.find("\r\n");
            while (line_end_pos == std::string::npos)
            {
                // 缓冲区中没有完整的行，需要从SSL连接中读取更多数据
                char temp_buffer[BUFFERSIZE + 1];
                memset(temp_buffer, '\0', sizeof(temp_buffer));
                int bytes_read = SSL_read(ssl_conn, temp_buffer, BUFFERSIZE);
                if (bytes_read <= 0)
                {
                    int err = SSL_get_error(ssl_conn, bytes_read);
                    if (err != SSL_ERROR_ZERO_RETURN)
                    {
                        HANDLE_ERROR("SSL read failed during chunk size reception");
                        SSL_free(ssl_conn); // Ensure SSL object is freed on error
                        throw std::runtime_error("SSL read failed during chunk size reception");
                    }
                    // 连接关闭或无更多数据，但可能还没有遇到最后一个0块
                    break; // 退出内部while循环
                }
                current_chunk_buffer.append(temp_buffer, bytes_read);
                line_end_pos = current_chunk_buffer.find("\r\n");
            }

            if (line_end_pos == std::string::npos)
            {          // 仍然没有完整行，可能连接已关闭或数据异常
                break; // 退出外层while循环
            }

            std::string chunk_size_str = current_chunk_buffer.substr(0, line_end_pos);
            // 移除分块大小后的扩展信息（如 ";chunk-extension"），只保留十六进制大小
            size_t semi_colon_pos = chunk_size_str.find(';');
            if (semi_colon_pos != std::string::npos)
            {
                chunk_size_str = chunk_size_str.substr(0, semi_colon_pos);
            }

            long chunk_size;
            try
            {
                chunk_size = std::stoul(chunk_size_str, nullptr, 16); // 将十六进制字符串转换为数字
            }
            catch (const std::exception &e)
            {
                log_e("Failed to parse chunk size '%s': {%s}\n", chunk_size_str.data(), e.what());
                SSL_free(ssl_conn); // Ensure SSL object is freed on error
                throw std::runtime_error("Failed to parse chunk size");
            }
            catch (...)
            {
                log_e("Failed to parse chunk size '%s' with unknown error\n", chunk_size_str.data());
                SSL_free(ssl_conn); // Ensure SSL object is freed on error
                throw std::runtime_error("Failed to parse chunk size with unknown error");
            }

            // 移除已解析的分块大小行（包括 CRLF）
            current_chunk_buffer = current_chunk_buffer.substr(line_end_pos + 2);

            if (chunk_size == 0)
            {
                // 遇到最后一个空块，表示分块数据结束
                // 还需要读取最后的 CRLF
                if (current_chunk_buffer.length() < 2)
                {
                    char temp_crlf[2];
                    SSL_read(ssl_conn, temp_crlf, 2); // 读取最后的 CRLF
                }
                else
                {
                    current_chunk_buffer = current_chunk_buffer.substr(2); // 移除最后的 CRLF
                }
                break; // 退出外层while循环
            }

            // 读取块数据
            long bytes_needed_for_chunk = chunk_size;
            while (bytes_needed_for_chunk > 0)
            {
                if (!current_chunk_buffer.empty())
                {
                    long bytes_from_buffer = std::min((long)current_chunk_buffer.length(), bytes_needed_for_chunk);
                    content.append(current_chunk_buffer.data(), bytes_from_buffer);
                    current_chunk_buffer = current_chunk_buffer.substr(bytes_from_buffer);
                    bytes_needed_for_chunk -= bytes_from_buffer;
                }
                else
                {
                    char data_buffer[BUFFERSIZE + 1];
                    memset(data_buffer, '\0', sizeof(data_buffer));
                    int bytes_read_data = SSL_read(ssl_conn, data_buffer, std::min((long)BUFFERSIZE, bytes_needed_for_chunk));
                    if (bytes_read_data <= 0)
                    {
                        int err = SSL_get_error(ssl_conn, bytes_read_data);
                        if (err != SSL_ERROR_ZERO_RETURN)
                        {
                            HANDLE_ERROR("SSL read failed during chunk data reception");
                        }
                        SSL_free(ssl_conn); // Ensure SSL object is freed on error
                        throw std::runtime_error("Incomplete chunk data: connection closed unexpectedly or SSL read error");
                    }
                    content.append(data_buffer, bytes_read_data);
                    bytes_needed_for_chunk -= bytes_read_data;
                }
            }

            // 读取每个块数据后的 CRLF
            if (current_chunk_buffer.length() < 2)
            {
                char temp_crlf[2];
                SSL_read(ssl_conn, temp_crlf, 2); // 读取 CRLF
            }
            else
            {
                current_chunk_buffer = current_chunk_buffer.substr(2); // 移除 CRLF
            }
        }
    }
    else
    {
        // 循环接收
        int content_size = 0; // 确保读取完整的响应内容
        try
        {
            content_size = getContentSize_Pimpl(response_buffer);
        }
        catch (const std::exception &e)
        {
            log_e("Failed to get content size: {%s}\n", e.what());
        }
        catch (...)
        {
            log_e("Failed to get content size: unknown error\n");
        }
        while (total_bytes_read < content_size)
        {
            memset(buffer, '\0', BUFFERSIZE + 1); // 清空缓冲区
            int bytes_read = SSL_read(ssl_conn, buffer, BUFFERSIZE);
            if (bytes_read == 0)
            {
                break; // 连接已关闭
            }
            else if (bytes_read < 0)
            {
                int err = SSL_get_error(ssl_conn, bytes_read);
                if (err != SSL_ERROR_ZERO_RETURN)
                { // SSL_ERROR_ZERO_RETURN 表示连接已关闭
                    HANDLE_ERROR("SSL read failed");
                    SSL_free(ssl_conn); // 释放 SSL 对象
                }
            }
            response_buffer.append(buffer, bytes_read);
            total_bytes_read += bytes_read;
        }

        // 解析响应头和内容
        size_t headerEnd = response_buffer.find("\r\n\r\n"); // 查找头部结束位置
        if (headerEnd == std::string::npos)
        {
            throw std::runtime_error("Invalid HTTP response format");
        }
        header = response_buffer.substr(0, headerEnd);   // 提取响应头
        content = response_buffer.substr(headerEnd + 4); // 提取响应内容
    }

    // 清理资源
    if (ssl_conn)
    {
        SSL_shutdown(ssl_conn); // 执行SSL关闭握手
        SSL_free(ssl_conn);     // 释放SSL对象 (也会释放关联的BIO)
    }
}

void cppNetworkUtilPimpl::run_Pimpl(serverCallback *callback, int http_port, int https_port)
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

#ifndef DISABLE_HTTPS
    // SSL 上下文
    if (!ssl_ctx_server)
    {
        HANDLE_ERROR("Unable to create SSL context");
        throw("Unable to create SSL context");
    }

    // 加载证书和私钥
    if (SSL_CTX_use_certificate_file(ssl_ctx_server, PUBLIC_KEY_PATH, SSL_FILETYPE_PEM) <= 0)
    {
        HANDLE_ERROR("Unable to load certificate PUBLIC KEY");
        throw("Unable to load certificate PUBLIC KEY");
    }
    if (SSL_CTX_use_PrivateKey_file(ssl_ctx_server, PRIVATE_KEY_PATH, SSL_FILETYPE_PEM) <= 0)
    {
        HANDLE_ERROR("Unable to load private key PRIVATE KEY");
        throw("Unable to load private key PRIVATE KEY");
    }
    // 验证私钥是否与证书匹配
    if (!SSL_CTX_check_private_key(ssl_ctx_server))
    {
        HANDLE_ERROR("Private key does not match the certificate");
        throw("Private key does not match the certificate");
    }
#endif

    // create socket
    SOCKET http_server_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (http_server_socket == INVALID_SOCKET)
    {
#ifdef _WIN32
        WSACleanup();
#endif
        throw("Create socket failed");
    }

    // set SO_REUSEADDR options
    bool set_SO_REUSEADDR_options_success = true;
    // 允许同时监听同个端口
    int optval = 1;
    if (setsockopt(http_server_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval)) == SOCKET_ERROR)
    {
        set_SO_REUSEADDR_options_success = false;
    }
    // 允许监听 IPv4 和 IPv6
    optval = 0;
    if (setsockopt(http_server_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&optval, sizeof(optval)) == SOCKET_ERROR)
    {
        set_SO_REUSEADDR_options_success = false;
    }
    if (!set_SO_REUSEADDR_options_success)
    {
        closesocket(http_server_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        throw("Setsockopt failed");
    }

#ifndef DISABLE_HTTPS
    SOCKET https_server_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (https_server_socket == INVALID_SOCKET)
    {
#ifdef _WIN32
        WSACleanup();
#endif
        throw("Create socket failed");
    }

    // set SO_REUSEADDR options
    set_SO_REUSEADDR_options_success = true;
    // 允许同时监听同个端口
    optval = 1;
    if (setsockopt(https_server_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval)) == SOCKET_ERROR)
    {
        set_SO_REUSEADDR_options_success = false;
    }
    // 允许监听 IPv4 和 IPv6
    optval = 0;
    if (setsockopt(https_server_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&optval, sizeof(optval)) == SOCKET_ERROR)
    {
        set_SO_REUSEADDR_options_success = false;
    }
    if (!set_SO_REUSEADDR_options_success)
    {
        closesocket(https_server_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        throw("Setsockopt failed");
    }
#endif

    // set server address
    // http
    struct sockaddr_in6 http_server_address;
    memset(&http_server_address, 0, sizeof(http_server_address));
    http_server_address.sin6_family = AF_INET6;       // use IPv6
    http_server_address.sin6_addr = in6addr_any;      // listen 0.0.0.0, all address
    http_server_address.sin6_port = htons(http_port); // set port
// https
#ifndef DISABLE_HTTPS
    struct sockaddr_in6 https_server_address;
    memset(&https_server_address, 0, sizeof(https_server_address));
    https_server_address.sin6_family = AF_INET6;        // use IPv6
    https_server_address.sin6_addr = in6addr_any;       // listen 0.0.0.0, all address
    https_server_address.sin6_port = htons(https_port); // set port
#endif

    // bind socket
    // http
    if (bind(http_server_socket, (const struct sockaddr *)&http_server_address, sizeof(http_server_address)) == SOCKET_ERROR)
    {
        closesocket(http_server_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        throw("Bind failed");
    }
    // https
#ifndef DISABLE_HTTPS
    if (bind(https_server_socket, (const struct sockaddr *)&https_server_address, sizeof(https_server_address)) == SOCKET_ERROR)
    {
        closesocket(http_server_socket);
        closesocket(https_server_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        throw("Bind failed");
    }
#endif

    // listen for connections
    if (listen(http_server_socket, SOMAXCONN) == SOCKET_ERROR)
    {
        closesocket(http_server_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        throw("Listen failed");
    }
#ifndef DISABLE_HTTPS
    if (listen(https_server_socket, SOMAXCONN) == SOCKET_ERROR)
    {
        closesocket(http_server_socket);
        closesocket(https_server_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        throw("Listen failed");
    }
#endif

#ifndef DISABLE_PRINT_LISTEN_INFO
#ifndef DISABLE_HTTPS
    printf("Listening on 0.0.0.0:%d\n", https_port);
#else
    printf("Listening on 0.0.0.0:%d\n", http_port);
#endif
#endif

    unsigned int cores = std::thread::hardware_concurrency();
    if (cores <= 0)
    {
        log_e("Unable to get the number of CPU cores\n");
        throw("Unable to get the number of CPU cores");
    }
    threadPool threadPool(cores); // 创建一个线程池

    threadPool.enqueue([this, http_server_socket, callback, http_port, https_port]()
                       { this->process(http_server_socket, callback, false, http_port, https_port); }); // 将处理函数添加到线程池中
#ifndef DISABLE_HTTPS
    threadPool.enqueue([this, https_server_socket, callback, http_port, https_port]()
                       { this->process(https_server_socket, callback, true, http_port, https_port); }); // 将处理函数添加到线程池中
#endif
}

void cppNetworkUtilPimpl::process(SOCKET server_socket, serverCallback *callback, bool enable_https, int http_port, int https_port)
{
    if (server_socket == INVALID_SOCKET)
    {
        throw("Invalid server socket");
    }
    if (!callback)
    {
        throw("No callback provided to process the request");
    }

    while (1)
    {
        struct sockaddr_storage client_address;
        socklen_t client_len = sizeof(client_address);
        SOCKET client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len);
        if (client_socket == INVALID_SOCKET)
        {
            log_e("Accept failed, errno: %d\n", errno);
            continue;
        }
        // log_d("enable_https=%d\n", (int)enable_https);

        ssl_st *ssl_conn = nullptr;
        if (enable_https)
        {
            // 为每个连接创建 SSL 结构并执行 SSL 握手
            ssl_conn = SSL_new(ssl_ctx_server);
            if (!ssl_conn)
            {
                HANDLE_ERROR("Unable to create SSL structure");
                // throw("Unable to create SSL structure"); //直接throw会退出循环
                continue; // 继续等待下一个连接
            }
            SSL_set_fd(ssl_conn, client_socket); // 将套接字描述符与 SSL 结构关联

            // 绑定 BIO 到套接字，告诉 OpenSSL 这个 BIO 不负责关闭底层套接字
            BIO *bio = BIO_new_socket(client_socket, BIO_NOCLOSE);
            SSL_set_bio(ssl_conn, bio, bio);

            // 执行 SSL/TLS 握手
            if (SSL_accept(ssl_conn) <= 0)
            {
                // 握手失败，打印 OpenSSL 错误信息
                HANDLE_ERROR("SSL accept failed");
                SSL_shutdown(ssl_conn);     // 尝试执行 SSL 关闭握手
                SSL_free(ssl_conn);         // 释放 SSL 结构
                closesocket(client_socket); // 关闭客户端套接字
                // throw("SSL accept failed");
                continue; // 继续等待下一个连接
            }
        }

        std::string request_data;

        int recvd = 0;
        int totla_recvd = 0;
        int content_size = 0;

        char temp_buffer[BUFFERSIZE + 1]; // +1 是为了 null 终止符，如果直接作为 C 字符串打印的话
        memset(temp_buffer, 0, BUFFERSIZE);
        // 接收数据
        if (enable_https)
        {
            recvd = SSL_read(ssl_conn, temp_buffer, BUFFERSIZE);
        }
        else
        {
            recvd = recv(client_socket, temp_buffer, BUFFERSIZE, 0);
        }
        if (recvd > 0)
        {
            // 只附加实际接收到的字节数
            request_data.append(temp_buffer, recvd);
            totla_recvd += recvd;
            // if (IS_DEBUG)
            //     std::cout << "recv " << recvd << " bytes, total recv: " << totla_recvd << " bytes\n";
        }
        else if (recvd == 0)
        {
            // 客户端已优雅断开连接
            continue; // 继续等待下一个连接
        }
        else
        {
            // 发生error
            log_e("An error occurred receiving, errno: %d\n", errno);
            if (enable_https)
            {
                SSL_shutdown(ssl_conn); // 尝试执行 SSL 关闭握手
                SSL_free(ssl_conn);     // 释放 SSL 结构
            }
            closesocket(client_socket); // 关闭客户端套接字
            break;                      // 继续等待下一个连接
        }

        try
        {
            content_size = getPostContentSize_Pimpl(request_data);
        }
        catch (const std::exception &e)
        {
            log_e("Get content size error: %s\n", e.what());
        }
        catch (...)
        {
            log_e("Get content size unknown error\n");
        }

        // 循环接收数据
        while (totla_recvd < content_size)
        {
            char temp_buffer_in_do_while[BUFFERSIZE + 1]; // +1 是为了 null 终止符，如果您直接作为 C 字符串打印的话
            memset(temp_buffer_in_do_while, 0, BUFFERSIZE);
            if (enable_https)
            {
                recvd = SSL_read(ssl_conn, temp_buffer_in_do_while, BUFFERSIZE);
            }
            else
            {
                recvd = recv(client_socket, temp_buffer_in_do_while, BUFFERSIZE, 0);
            }
            if (recvd > 0)
            {
                // 只附加实际接收到的字节数
                request_data.append(temp_buffer_in_do_while, recvd);
                totla_recvd += recvd;
            }
            else if (recvd == 0)
            {
                // 客户端已优雅断开连接
                break; // 跳出循环
            }
            else
            {
                // 发生error
                log_e("An error occurred receiving, errno: %d\n", errno);
                break;
            }
        }

        // 分离请求体
        size_t header_end_pos = request_data.find("\r\n\r\n");
        std::string request_header, request_content;
        if (header_end_pos != std::string::npos)
        {
            request_header = request_data.substr(0, header_end_pos);
            request_content = request_data.substr(header_end_pos + 4);
        }
        else
        {
            request_header = request_data;
            request_content = "";
        }

#ifndef DISABLE_HTTPS
        if (!enable_https)
        {
            std::map<std::string, std::string> parsed_header = getParsedHeader_Pimpl(request_header);

            std::string Location;
            Location += "https://";
            Location += parsed_header["Host"];
            if (!parsed_header["url"].empty())
                Location += "/";
            Location += parsed_header["url"];
            if (https_port != 443)
            {
                Location += ":";
                Location += std::to_string(https_port);
            }
            // log_d("Location=%s\n", Location.data());
            sendDataToHttpSocket_Pimpl(client_socket, makeResponseHeader_Pimpl({{"status", "301"}, {"connection", "close"}, {"Location", Location}}));
            closesocket(client_socket);
            continue;
        }
#endif

        if (enable_https)
        {
            client_connections[client_socket].ssl = ssl_conn; // 将 SSL 结构存储到 client_connections 中
        }
        client_connections[client_socket].is_https_connection = enable_https;
        if (client_address.ss_family == AF_INET)
        {
            // IPv4 连接
            struct sockaddr_in *ipv4_addr = (struct sockaddr_in *)&client_address;
            inet_ntop(AF_INET, &(ipv4_addr->sin_addr), client_connections[client_socket].ip.data(), client_connections[client_socket].ip.size());
            client_connections[client_socket].port = ntohs(ipv4_addr->sin_port);
            client_connections[client_socket].family = "ipv4";
        }
        else if (client_address.ss_family == AF_INET6)
        {
            // IPv6 连接
            struct sockaddr_in6 *ipv6_addr = (struct sockaddr_in6 *)&client_address;
            inet_ntop(AF_INET6, &(ipv6_addr->sin6_addr), client_connections[client_socket].ip.data(), client_connections[client_socket].ip.size());
            client_connections[client_socket].port = ntohs(ipv6_addr->sin6_port);
            client_connections[client_socket].family = "ipv6";
        }
        client_connections[client_socket].request_data = request_data; // 将接收到的数据存储到 client_connections 中
        client_connections[client_socket].request_header = request_header;
        client_connections[client_socket].request_content = request_content;

        // 调用用户的函数
        if (callback)
        {
            callback->onDataReceived(client_socket);
        }
        else
        {
            log_e("No callback provided to process the request\n");
        }
        if (enable_https)
        {
            SSL_shutdown(ssl_conn); // 尝试执行 SSL 关闭握手
            SSL_free(ssl_conn);     // 释放 SSL 结构
        }
        closesocket(client_socket); // 关闭套接字
    }
}

void cppNetworkUtilPimpl::print_cppNetworkUtilVersion_Pimpl()
{
    std::cout << "cppNetworkUtil version: " << VERSION << "\n";
}

void cppNetworkUtilPimpl::print_opensslVersion_Pimpl()
{
    std::cout << "openSSL version: " << OpenSSL_version(OPENSSL_VERSION) << "\n";
}