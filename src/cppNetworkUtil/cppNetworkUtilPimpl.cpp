#include "cppNetworkUtilPimpl.h"

#include <openssl/err.h>
#include <openssl/ssl.h>

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

    // 删除所有客户端连接信息
    std::unordered_map<SOCKET, std::unordered_map<int, clientConnectionInfo_Pimpl>>().swap(client_connections);
}

bool cppNetworkUtilPimpl::isRegexPattern_Pimpl(const std::string &s)
{
    // 检查特殊字符 或 字符串是否是 "*"
    return s.find_first_of("[]()|.^$+?") != std::string::npos || s == "*";
}

bool cppNetworkUtilPimpl::isLocalIpAddress_Pimpl(const std::string &ip_str)
{
    struct in_addr ipv4_addr;
    struct in6_addr ipv6_addr;

    // --- 1. 尝试解析为 IPv4 地址 ---
    int ipv4_result = inet_pton(AF_INET, ip_str.c_str(), &ipv4_addr);
    if (ipv4_result == 1)
    {
        // 解析成功，进行 IPv4 私有地址判断

        // 将网络字节序的地址转换为宿主字节序，便于按位比较
        uint32_t ip_host = ntohl(ipv4_addr.s_addr);

        // A. 10.0.0.0/8
        if ((ip_host >> 24) == 10)
        {
            return true;
        }

        // B. 172.16.0.0/12 (172.16.0.0 - 172.31.255.255)
        if ((ip_host & 0xFFF00000) == 0xAC100000)
        {
            return true;
        }

        // C. 192.168.0.0/16
        if ((ip_host & 0xFFFF0000) == 0xC0A80000)
        {
            return true;
        }

        // D. Link-Local (APIPA) 169.254.0.0/16
        if ((ip_host & 0xFFFF0000) == 0xA9FE0000)
        {
            return true;
        }

        // E. 排除环回地址 127.0.0.1/8
        if ((ip_host >> 24) == 127)
        {
            return false;
        }

        return false; // 不是私有 IPv4 地址
    }

    // --- 2. 尝试解析为 IPv6 地址 ---
    int ipv6_result = inet_pton(AF_INET6, ip_str.c_str(), &ipv6_addr);
    if (ipv6_result == 1)
    {
        // 解析成功，进行 IPv6 私有地址判断
        const uint8_t *ip_bytes = ipv6_addr.s6_addr;

        // A. Unique Local Address (ULA) fc00::/7
        // 检查第一个字节 AND 0xFE 是否等于 0xFC (即 fc 或 fd)
        if ((ip_bytes[0] & 0xFE) == 0xFC)
        {
            return true;
        }

        // B. Link-Local Address fe80::/10
        // 第一个字节是 0xFE，第二个字节的高两位是 10 (即 0x80 到 0xBF)
        if (ip_bytes[0] == 0xFE && (ip_bytes[1] & 0xC0) == 0x80)
        {
            return true;
        }

        return false; // 不是私有 IPv6 地址
    }

    // --- 3. 解析失败 ---
    // IP 地址格式错误或为空
    return false;
}

std::unordered_map<std::string, std::string> cppNetworkUtilPimpl::getParsedHeader_Pimpl(const std::string &header)
{
    std::unordered_map<std::string, std::string> parsed_header;
    parsed_header.reserve(16);

    // 1) 请求行解析：Method URL HTTP-Version
    size_t pos = 0;
    size_t first_space = header.find(' ', pos);
    if (first_space == std::string::npos)
        throw std::runtime_error("Invalid request line: No space found after method");

    parsed_header["method"] = header.substr(0, first_space);

    size_t second_space = header.find(' ', first_space + 1);
    if (second_space == std::string::npos)
        throw std::runtime_error("Invalid request line: No space found after url");

    parsed_header["url"] = header.substr(first_space + 1, second_space - first_space - 1);

    size_t line_end = header.find("\r\n", second_space + 1);
    if (line_end == std::string::npos)
        throw std::runtime_error("Invalid request line: No \\r\\n found after http version");

    parsed_header["http_version"] = header.substr(second_space + 1, line_end - second_space - 1);

    // 2) 头部字段解析（从请求行后的 CRLF 开始）
    size_t headers_start = line_end + 2; // 跳过请求行的 "\r\n"
    size_t headers_end = header.find("\r\n\r\n", headers_start);
    if (headers_end == std::string::npos)
        headers_end = header.length();

    size_t cur = headers_start;
    while (cur < headers_end)
    {
        // 找到下一行的结束（以 '\n' 为界），兼容 "\r\n" 与 "\n"
        size_t next_n = header.find('\n', cur);
        size_t line_end_pos = (next_n == std::string::npos || next_n > headers_end) ? headers_end : next_n;

        // 计算行的起止索引
        size_t line_start = cur;
        size_t line_len = (line_end_pos > line_start) ? (line_end_pos - line_start) : 0;

        // 跳过空行
        if (line_len == 0)
        {
            cur = (line_end_pos == headers_end) ? headers_end : (line_end_pos + 1);
            continue;
        }

        // 去掉行尾可能的 '\r'
        size_t real_end = line_start + line_len;
        if (real_end > line_start && header[real_end - 1] == '\r')
            --real_end;

        // 查找冒号分隔 key:value
        size_t colon = header.find(':', line_start);
        if (colon != std::string::npos && colon < real_end)
        {
            // key = [line_start, colon)
            size_t key_first = line_start;
            size_t key_last = colon; // exclusive

            // trim key (前后空白)
            while (key_first < key_last && (header[key_first] == ' ' || header[key_first] == '\t'))
                ++key_first;
            while (key_last > key_first && (header[key_last - 1] == ' ' || header[key_last - 1] == '\t'))
                --key_last;

            // value = (colon+1) .. real_end
            size_t value_first = colon + 1;
            size_t value_last = real_end; // exclusive

            // trim value (前后空白)
            while (value_first < value_last && (header[value_first] == ' ' || header[value_first] == '\t'))
                ++value_first;
            while (value_last > value_first && (header[value_last - 1] == ' ' || header[value_last - 1] == '\t'))
                --value_last;

            if (key_first < key_last)
            {
                std::string key = header.substr(key_first, key_last - key_first);
                std::string value =
                    (value_first < value_last) ? header.substr(value_first, value_last - value_first) : std::string();
                parsed_header.emplace(std::move(key), std::move(value));
            }
        }

        // 移动到下一行（跳过 '\n'）
        cur = (line_end_pos == headers_end) ? headers_end : (line_end_pos + 1);
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

bool cppNetworkUtilPimpl::ensureBuffer_Pimpl(SOCKET sock, ssl_st *ssl_conn, std::string &buffer, size_t targetSize)
{
    char temp[4096];
    while (buffer.length() < targetSize)
    {
        int bytes;
        if (ssl_conn)
        {
            // 使用 SSL_read 读取数据
            bytes = SSL_read(ssl_conn, temp, sizeof(temp));
            if (bytes <= 0)
            {
                int ssl_error = SSL_get_error(ssl_conn, bytes);
                if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE)
                {
                    // 非阻塞模式下，继续等待
                    continue;
                }
                else
                {
                    // 其他错误
                    return false;
                }
            }
        }
        else
        {
            // 使用普通的 recv 读取数据
            bytes = recv(sock, temp, sizeof(temp), 0);
            if (bytes <= 0)
            {
                return false; // 断开或错误
            }
        }
        if (bytes <= 0)
            return false; // 断开或错误
        buffer.append(temp, bytes);
    }
    return true;
}

bool cppNetworkUtilPimpl::ensureBufferUntil_Pimpl(SOCKET sock, ssl_st *ssl_conn, std::string &buffer,
                                                  const std::string &target)
{
    char temp[BUFFERSIZE];
    while (buffer.find(target) == std::string::npos)
    {
        int bytes;
        if (ssl_conn)
        {
            // 使用 SSL_read 读取数据
            bytes = SSL_read(ssl_conn, temp, BUFFERSIZE);
            if (bytes <= 0)
            {
                int ssl_error = SSL_get_error(ssl_conn, bytes);
                if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE)
                {
                    // 非阻塞模式下，继续等待
                    continue;
                }
                else
                {
                    // 其他错误
                    return false;
                }
            }
        }
        else
        {
            // 使用普通的 recv 读取数据
            bytes = recv(sock, temp, BUFFERSIZE, 0);
            if (bytes <= 0)
            {
                return false; // 断开或错误
            }
        }
        if (bytes <= 0)
            return false;
        buffer.append(temp, bytes);
    }
    return true;
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

std::string cppNetworkUtilPimpl::getTransferEncodingValue_Pimpl(const std::string &headers)
{
    std::string transferEncoding = getHeaderValue_Pimpl(headers, "Transfer-Encoding");
    return transferEncoding;
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
    while (pos < buffer.length() && buffer[pos] != ';' && buffer[pos] != '\r' && buffer[pos] != '\n' &&
           buffer[pos] != '\0')
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

std::optional<std::string> cppNetworkUtilPimpl::getHttpCodeText_Pimpl(int status_code)
{
    auto it = http_code.find(status_code);
    if (it != http_code.end())
    {
        return it->second;
    }
    else
    {
        return std::nullopt; // 如果未找到，返回空
    }
}

std::optional<std::string> cppNetworkUtilPimpl::getMimeType_Pimpl(const std::string &file_extension)
{
    auto it = mimeTypeMap.find(file_extension);

    if (it != mimeTypeMap.end())
    {
        return it->second;
    }
    else
    {
        return std::nullopt; // 如果未找到，返回空
    }
}

std::string cppNetworkUtilPimpl::makeResponseHeader_Pimpl(int status_code,
                                                          std::unordered_map<std::string, std::string> parameters)
{
    std::string buffer;

    std::optional<std::string> title = getHttpCodeText_Pimpl(status_code);

    buffer += PROTOCOL;
    buffer += " ";
    buffer += std::to_string(status_code);
    buffer += " ";
    buffer += title.value_or("Unknown");
    buffer += "\r\n";

    if (parameters.count("connection"))
    {
        buffer += "Connection: ";
        buffer += parameters["connection"];
        buffer += "\r\n";
    }
    else
    {
        if (PROTOCOL == "HTTP/1.1")
        {
            buffer += "Connection: keep-alive\r\n";
        }
        else
        {
            buffer += "Connection: close\r\n";
        }
    }

    char *timebuf = new char[100];
    time_t now_date = time((time_t *)0);
    strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now_date));

    buffer += "Server: ";
    buffer += NAME;
    buffer += "\r\n";

    for (const auto &pair : parameters)
    {
        if (pair.first != "status_code" && pair.first != "connection" && pair.first != "" && pair.second != "")
            buffer += pair.first + ": " + pair.second + "\r\n";
    }
    buffer += "\r\n";

    delete[] timebuf;

    return buffer;
}

std::string cppNetworkUtilPimpl::makeRequestHeader_Pimpl(std::unordered_map<std::string, std::string> parameters)
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
        buffer += "Connection: keep-alive\r\n";
    }

    for (const auto &kv : parameters)
    {
        if (kv.first == "method" || kv.first == "path" || kv.first == "host" || kv.first == "port" ||
            kv.first == "connection")
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

std::unordered_map<std::string, std::string> cppNetworkUtilPimpl::parseUrlEncodedFormBody_Pimpl(
    const std::string &postBody)
{
    std::unordered_map<std::string, std::string> formData;

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

// 切分url路径
std::vector<std::string> cppNetworkUtilPimpl::cutUrlPath_Pimpl(std::string path)
{
    std::vector<std::string> parts;

    // 如果路径以斜杠开头，则去除它
    if (!path.empty() && path[0] == '/')
    {
        path = path.substr(1);
    }

    std::stringstream ss(path);
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

std::unordered_map<std::string, std::string> cppNetworkUtilPimpl::parseUrlQueryParameters_Pimpl(const std::string &url)
{
    std::unordered_map<std::string, std::string> params;
    size_t question_pos = url.find('?');
    if (question_pos == std::string::npos || question_pos + 1 >= url.length())
        return params;

    // Use string_view to avoid intermediate substring allocations
    std::string_view query(url.data() + question_pos + 1, url.size() - question_pos - 1);

    // Quick estimate for reserving map buckets
    size_t pair_count = 1;
    for (char c : query)
        if (c == '&')
            ++pair_count;
    params.reserve(pair_count * 2);

    // fast hex digit -> value
    auto hex_val = [](char c) -> int {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'A' && c <= 'F')
            return 10 + (c - 'A');
        if (c >= 'a' && c <= 'f')
            return 10 + (c - 'a');
        return -1;
    };

    // decode percent-encoding from a string_view into a std::string
    auto decode_sv = [&](std::string_view sv) -> std::string {
        std::string out;
        out.reserve(sv.size());
        for (size_t i = 0; i < sv.size(); ++i)
        {
            char ch = sv[i];
            if (ch == '+')
            {
                out.push_back(' ');
            }
            else if (ch == '%' && i + 2 < sv.size())
            {
                int hi = hex_val(sv[i + 1]);
                int lo = hex_val(sv[i + 2]);
                if (hi >= 0 && lo >= 0)
                {
                    out.push_back(static_cast<char>((hi << 4) | lo));
                    i += 2;
                }
                else
                {
                    // invalid percent-encoding, keep '%' literally
                    out.push_back('%');
                }
            }
            else
            {
                out.push_back(ch);
            }
        }
        return out;
    };

    size_t pos = 0;
    while (pos < query.size())
    {
        size_t amp = query.find('&', pos);
        size_t end = (amp == std::string_view::npos) ? query.size() : amp;

        if (end == pos)
        {
            // empty pair, skip
            pos = (amp == std::string_view::npos) ? query.size() : amp + 1;
            continue;
        }

        size_t eq = query.find('=', pos);
        if (eq == std::string_view::npos || eq > end)
        {
            // key only
            std::string key = decode_sv(query.substr(pos, end - pos));
            params.emplace(std::move(key), std::string());
        }
        else
        {
            std::string key = decode_sv(query.substr(pos, eq - pos));
            std::string value = decode_sv(query.substr(eq + 1, end - (eq + 1)));
            params.emplace(std::move(key), std::move(value));
        }

        pos = (amp == std::string_view::npos) ? query.size() : amp + 1;
    }

    return params;
}

// 解析 multipart 数据
std::unordered_map<std::string, multipartData> cppNetworkUtilPimpl::parseMultipart_Pimpl(const std::string &boundary,
                                                                                         const std::string &body)
{
    std::unordered_map<std::string, multipartData> parsedParts;
    if (boundary.empty() || body.empty())
        return parsedParts;

    std::string_view sv(body);
    // build an owning string for the delimiter and create a string_view that refers to it
    std::string delim_str = std::string("--") + boundary;
    std::string_view delim(delim_str);
    size_t delim_len = delim.size();

    size_t pos = sv.find(delim);
    if (pos == std::string_view::npos)
        return parsedParts;

    // helper trims leading/trailing spaces/tabs
    auto trim_sv = [](std::string_view v) -> std::string_view {
        size_t a = 0;
        while (a < v.size() && (v[a] == ' ' || v[a] == '\t'))
            ++a;
        size_t b = v.size();
        while (b > a && (v[b - 1] == ' ' || v[b - 1] == '\t'))
            --b;
        return v.substr(a, b - a);
    };

    pos += delim_len;
    while (true)
    {
        // Skip optional CRLF (start of part)
        while (pos < sv.size() && (sv[pos] == '\r' || sv[pos] == '\n'))
            ++pos;

        // Check for final boundary marker "--" immediately after delimiter
        if (pos + 2 <= sv.size() && sv.substr(pos, 2) == std::string_view("--"))
            break;

        size_t next_delim = sv.find(delim, pos);
        if (next_delim == std::string_view::npos)
            break; // malformed or end

        // Find headers end (CRLFCRLF or LFLF) but ensure it is before next_delim
        size_t headers_end = sv.find("\r\n\r\n", pos);
        size_t sep_len = 4;
        if (headers_end == std::string_view::npos || headers_end > next_delim)
        {
            headers_end = sv.find("\n\n", pos);
            sep_len = 2;
        }
        if (headers_end == std::string_view::npos || headers_end > next_delim)
        {
            pos = next_delim;
            continue; // skip malformed part
        }

        std::string_view headers_sv = sv.substr(pos, headers_end - pos);
        size_t data_start = headers_end + sep_len;
        size_t data_end = next_delim;

        // Trim trailing CRLF immediately before the delimiter
        while (data_end > data_start && (sv[data_end - 1] == '\r' || sv[data_end - 1] == '\n'))
            --data_end;

        std::string name;
        multipartData partData;

        // Parse headers line by line to extract Content-Disposition (name, filename) and Content-Type
        size_t line_pos = 0;
        while (line_pos < headers_sv.size())
        {
            // find end of line (CRLF or LF)
            size_t line_end = headers_sv.find("\r\n", line_pos);
            size_t line_sep_len = 2;
            if (line_end == std::string_view::npos)
            {
                line_end = headers_sv.find('\n', line_pos);
                line_sep_len = 1;
            }
            if (line_end == std::string_view::npos)
            {
                line_end = headers_sv.size();
                line_sep_len = 0;
            }

            std::string_view line = headers_sv.substr(line_pos, line_end - line_pos);
            // find colon
            size_t colon = line.find(':');
            if (colon != std::string_view::npos)
            {
                std::string key;
                key.resize(colon);
                for (size_t i = 0; i < colon; ++i)
                    key[i] = static_cast<char>(::tolower(line[i]));
                std::string_view value = trim_sv(line.substr(colon + 1));

                if (key == "content-disposition")
                {
                    // parse parameters like name="..." ; filename="..."
                    // scan value for tokens
                    size_t i = 0;
                    while (i < value.size())
                    {
                        // skip spaces and semicolons
                        while (i < value.size() && (value[i] == ' ' || value[i] == ';'))
                            ++i;
                        // read token
                        size_t kstart = i;
                        while (i < value.size() && value[i] != '=' && value[i] != ';')
                            ++i;
                        std::string_view token = value.substr(kstart, (i > kstart ? i - kstart : 0));
                        // skip '='
                        if (i < value.size() && value[i] == '=')
                        {
                            ++i;
                            // value may be quoted
                            std::string_view val;
                            if (i < value.size() && value[i] == '"')
                            {
                                ++i;
                                size_t vstart = i;
                                while (i < value.size() && value[i] != '"')
                                    ++i;
                                val = value.substr(vstart, i - vstart);
                                if (i < value.size() && value[i] == '"')
                                    ++i;
                            }
                            else
                            {
                                size_t vstart = i;
                                while (i < value.size() && value[i] != ';')
                                    ++i;
                                val = value.substr(vstart, i - vstart);
                            }

                            // lowercase token for comparison
                            std::string token_l;
                            token_l.resize(token.size());
                            for (size_t j = 0; j < token.size(); ++j)
                                token_l[j] = static_cast<char>(::tolower(token[j]));

                            if (token_l == "name")
                            {
                                name = std::string(trim_sv(val));
                            }
                            else if (token_l == "filename")
                            {
                                partData.filename = std::string(trim_sv(val));
                            }
                        }
                        else
                        {
                            // no '=' skip to next semicolon
                            while (i < value.size() && value[i] != ';')
                                ++i;
                        }
                    }
                }
                else if (key == "content-type")
                {
                    partData.content_type = std::string(trim_sv(value));
                }
            }

            line_pos = line_end + line_sep_len;
        }

        // default name if none (use index or empty)
        if (name.empty())
            name = std::to_string(parsedParts.size());

        // copy data once
        size_t len = (data_end > data_start) ? (data_end - data_start) : 0;
        if (len)
            partData.data.assign(sv.data() + data_start, len);
        else
            partData.data.clear();

        parsedParts.emplace(std::move(name), std::move(partData));

        pos = next_delim;
    }

    return parsedParts;
}

void cppNetworkUtilPimpl::sendDataToHttpSocket_Pimpl(SOCKET socket, const std::string &data)
{
    send(socket, data.data(), data.length(), 0); // 发送数据到套接字
}

void cppNetworkUtilPimpl::sendDataToHttpsSocket_Pimpl(SOCKET socket, int request_count, const std::string &data)
{
    SSL_write(client_connections[socket][request_count].ssl, data.data(), data.length()); // 发送数据到 SSL 套接字
}

void cppNetworkUtilPimpl::sendDataToHttpsSocket_Pimpl(ssl_st *ssl, const std::string &data)
{
    SSL_write(ssl, data.data(), data.length()); // 发送数据到 SSL 套接字
}

void cppNetworkUtilPimpl::sendDataToSocket_Pimpl(SOCKET socket, int request_count, const std::string &data)
{
    if (client_connections[socket][request_count].is_https_connection)
    {
        sendDataToHttpsSocket_Pimpl(socket, request_count, data);
    }
    else
    {
        sendDataToHttpSocket_Pimpl(socket, data);
    }
}

void cppNetworkUtilPimpl::sendDataChunkToSocket_Pimpl(SOCKET socket, int request_count, const std::string &data)
{
    size_t chunk_size = data.length(); // 数据块大小

    std::stringstream chunk_size_stream;                  // 创建字符串流
    chunk_size_stream << std::hex << chunk_size;          // 转换为十六进制字符串
    std::string chunk_size_str = chunk_size_stream.str(); // 获取字符串

    sendDataToSocket_Pimpl(socket, request_count, chunk_size_str); // 发送块大小
    sendDataToSocket_Pimpl(socket, request_count, "\r\n");         // 发送 CRLF

    sendDataToSocket_Pimpl(socket, request_count, data);   // 发送数据块
    sendDataToSocket_Pimpl(socket, request_count, "\r\n"); // 发送 CRLF
}

void cppNetworkUtilPimpl::sendDataToHttpHost_Pimpl(const std::string &host, const std::string &path, int port,
                                                   const std::unordered_map<std::string, std::string> &request_header,
                                                   std::string &header, std::string &content)
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

    // 发送请求头
    std::string request_header_str = makeRequestHeader_Pimpl(request_header);
    sendDataToHttpSocket_Pimpl(sock, request_header_str);

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
        try
        {
            // 定义 Lambda 捕获 sock，封装底层的 recv 读取操作
            auto http_read_func = [sock](char *read_buffer, int len) -> int { return recv(sock, read_buffer, len, 0); };

            // 调用通用的解码函数
            decodeChunkedResponse_Pimpl(remaining_buffer_after_header, content, http_read_func);
        }
        catch (const std::runtime_error &e)
        {
            // 捕获解码函数中抛出的异常并处理资源清理
            HANDLE_ERROR(e.what());
            closesocket(sock);
#ifdef _WIN32
            WSACleanup();
#endif
            throw; // 重新抛出异常
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

void cppNetworkUtilPimpl::sendDataToHttpsHost_Pimpl(const std::string &host, const std::string &path, int port,
                                                    const std::unordered_map<std::string, std::string> &request_header,
                                                    std::string &header, std::string &content, bool enable_CA)
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

    // 发送请求
    std::string request_header_str = makeRequestHeader_Pimpl(request_header);
    sendDataToHttpsSocket_Pimpl(ssl, request_header_str);

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
        try
        {
            // 定义 Lambda 捕获 ssl_conn，封装底层的 SSL_read 读取操作
            auto https_read_func = [ssl_conn](char *read_buffer, int len) -> int {
                int bytes_read = SSL_read(ssl_conn, read_buffer, len);
                if (bytes_read <= 0)
                {
                    int err = SSL_get_error(ssl_conn, bytes_read);
                    if (err != SSL_ERROR_ZERO_RETURN)
                    {
                        HANDLE_ERROR("SSL read failed inside chunk handler");
                    }
                }
                return bytes_read;
            };

            // 调用通用的解码函数
            decodeChunkedResponse_Pimpl(remaining_buffer_after_header, content, https_read_func);
        }
        catch (const std::runtime_error &e)
        {
            HANDLE_ERROR(e.what());
            if (ssl_conn)
            {
                SSL_free(ssl_conn);
            }
#ifdef _WIN32
            WSACleanup();
#endif
            throw;
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
        size_t header_end = response_buffer.find("\r\n\r\n"); // 查找头部结束位置
        if (header_end == std::string::npos)
        {
            throw std::runtime_error("Invalid HTTP response format");
        }
        header = response_buffer.substr(0, header_end);   // 提取响应头
        content = response_buffer.substr(header_end + 4); // 提取响应内容
    }

    // 清理资源
    if (ssl_conn)
    {
        SSL_shutdown(ssl_conn); // 执行SSL关闭握手
        SSL_free(ssl_conn);     // 释放SSL对象 (也会释放关联的BIO)
    }
}

void cppNetworkUtilPimpl::decodeChunkedResponse_Pimpl(std::string &current_chunk_buffer, std::string &content,
                                                      std::function<int(char *, int)> read_func)
{
    content.clear();

    while (true)
    {
        size_t line_end_pos = current_chunk_buffer.find("\r\n");
        while (line_end_pos == std::string::npos)
        {
            char temp_buffer[BUFFERSIZE + 1];
            memset(temp_buffer, '\0', sizeof(temp_buffer));

            // 使用抽象的读取函数
            int bytes_read_chunk = read_func(temp_buffer, BUFFERSIZE);

            if (bytes_read_chunk <= 0)
            {
                throw std::runtime_error("Socket read failed during chunk size reception");
            }
            current_chunk_buffer.append(temp_buffer, bytes_read_chunk);
            line_end_pos = current_chunk_buffer.find("\r\n");
        }

        if (line_end_pos == std::string::npos)
        {
            break;
        }

        std::string chunk_size_str = current_chunk_buffer.substr(0, line_end_pos);
        size_t semi_colon_pos = chunk_size_str.find(';');
        if (semi_colon_pos != std::string::npos)
        {
            chunk_size_str = chunk_size_str.substr(0, semi_colon_pos);
        }

        long chunk_size;
        try
        {
            chunk_size = std::stoul(chunk_size_str, nullptr, 16);
        }
        catch (const std::exception &e)
        {
            log_e("Failed to parse chunk size '%s': {%s}\n", chunk_size_str.data(), e.what());
            throw std::runtime_error("Failed to parse chunk size");
        }
        catch (...)
        {
            log_e("Failed to parse chunk size '%s' with unknown error\n", chunk_size_str.data());
            throw std::runtime_error("Failed to parse chunk size with unknown error");
        }

        current_chunk_buffer = current_chunk_buffer.substr(line_end_pos + 2);

        if (chunk_size == 0)
        {
            // 读取最后的 CRLF
            if (current_chunk_buffer.length() < 2)
            {
                char temp_crlf[2];
                read_func(temp_crlf, 2);
            }
            else
            {
                current_chunk_buffer = current_chunk_buffer.substr(2);
            }
            break;
        }

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
                int bytes_to_read = std::min((long)BUFFERSIZE, bytes_needed_for_chunk);

                // 使用抽象读取函数
                int bytes_read_data = read_func(data_buffer, bytes_to_read);

                if (bytes_read_data <= 0)
                {
                    throw std::runtime_error(
                        "Incomplete chunk data: connection closed unexpectedly or socket read error");
                }
                content.append(data_buffer, bytes_read_data);
                bytes_needed_for_chunk -= bytes_read_data;
            }
        }

        // 读取每个块数据后的 CRLF
        if (current_chunk_buffer.length() < 2)
        {
            char temp_crlf[2];
            read_func(temp_crlf, 2);
        }
        else
        {
            current_chunk_buffer = current_chunk_buffer.substr(2);
        }
    }
}

bool cppNetworkUtilPimpl::parseChunkedBody_Pimpl(SOCKET sock, ssl_st *ssl_conn, std::string &buffer,
                                                 std::string &outBody)
{
    size_t pos = 0;
    char temp[BUFFERSIZE];

    auto read_more = [&](int need = BUFFERSIZE) -> bool {
        int bytes;
        if (ssl_conn)
        {
            bytes = SSL_read(ssl_conn, temp, need);
            if (bytes <= 0)
            {
                int ssl_error = SSL_get_error(ssl_conn, bytes);
                if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE)
                    return true; // treat as try again
                return false;
            }
        }
        else
        {
            bytes = recv(sock, temp, need, 0);
            if (bytes <= 0)
                return false;
        }
        buffer.append(temp, bytes);
        return true;
    };

    try
    {
        while (true)
        {
            // 1) 找到当前 chunk size 行的 CRLF（从 pos 开始查找）
            size_t crlfPos = buffer.find("\r\n", pos);
            while (crlfPos == std::string::npos)
            {
                if (!read_more())
                    return false;
                crlfPos = buffer.find("\r\n", pos);
            }

            // 解析 size 行（支持 ;comment）
            std::string sizeLine = buffer.substr(pos, crlfPos - pos);
            size_t semi = sizeLine.find(';');
            if (semi != std::string::npos)
                sizeLine.resize(semi);

            // 前进到数据起点（跳过 size line + CRLF）
            pos = crlfPos + 2;

            // 2) 解析十六进制大小
            unsigned long chunkSize = 0;
            try
            {
                // trim possible leading/trailing spaces
                size_t first = sizeLine.find_first_not_of(" \t");
                size_t last = sizeLine.find_last_not_of(" \t");
                if (first == std::string::npos)
                    return false;
                std::string trimmed = sizeLine.substr(first, last - first + 1);
                chunkSize = std::stoul(trimmed, nullptr, 16);
            }
            catch (...)
            {
                log_e("Chunk size parse error: %s", sizeLine.data());
                return false;
            }

            // 3) 结束标志
            if (chunkSize == 0)
            {
                // 末尾会有一个 CRLF（以及可选的 trailers，但这里只跳过最后的 CRLF）
                while (buffer.size() < pos + 2)
                {
                    if (!read_more())
                        return false;
                }
                pos += 2;
                // 清理已消费的数据
                if (pos >= buffer.size())
                {
                    buffer.clear();
                }
                else
                {
                    buffer.erase(0, pos);
                }
                return true;
            }

            // 4) 确保整个 chunk + trailing CRLF 已经在 buffer 中
            while (buffer.size() < pos + chunkSize + 2)
            {
                if (!read_more())
                    return false;
            }

            // 5) 追加 chunk 数据到 outBody（避免中间字符串分配）
            outBody.append(buffer.data() + pos, chunkSize);

            // 6) 移动 pos 越过 chunk 数据和后面的 CRLF
            pos += chunkSize + 2;

            // 7) 如果已消费前缀较大，批量擦除以避免字符串无限增长和频繁移动
            if (pos > 4096)
            {
                buffer.erase(0, pos);
                pos = 0;
            }
        }
    }
    catch (...)
    {
        return false;
    }
}

SOCKET cppNetworkUtilPimpl::createAndBindSocket_Pimpl(int port, int ip_protocol_family, bool is_ipv6_only)
{
    SOCKET server_socket = socket(ip_protocol_family, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET)
    {
        throw std::runtime_error("Create socket failed");
    }

    bool set_options_success = true;
    int optval = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval)) == SOCKET_ERROR)
    {
        set_options_success = false;
    }

    if (ip_protocol_family == AF_INET6)
    {
        // 设置 IPV6_V6ONLY
        optval = is_ipv6_only ? 1 : 0;
        if (setsockopt(server_socket, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&optval, sizeof(optval)) == SOCKET_ERROR)
        {
            set_options_success = false;
        }
    }

    if (!set_options_success)
    {
        closesocket(server_socket);
        throw std::runtime_error("Setsockopt failed");
    }

    if (ip_protocol_family == AF_INET6)
    {
        struct sockaddr_in6 server_address;
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin6_family = AF_INET6;
        server_address.sin6_addr = in6addr_any;
        server_address.sin6_port = htons(port);
        if (bind(server_socket, (const struct sockaddr *)&server_address, sizeof(server_address)) == SOCKET_ERROR)
        {
            closesocket(server_socket);
            throw std::runtime_error("Bind failed");
        }
    }
    else
    {
        struct sockaddr_in server_address;
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = INADDR_ANY;
        server_address.sin_port = htons(port);
        if (bind(server_socket, (const struct sockaddr *)&server_address, sizeof(server_address)) == SOCKET_ERROR)
        {
            closesocket(server_socket);
            throw std::runtime_error("Bind failed");
        }
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR)
    {
        closesocket(server_socket);
        throw std::runtime_error("Listen failed");
    }

    return server_socket;
}

void cppNetworkUtilPimpl::run_Pimpl(int http_port, int https_port, int behavior_mode, int ip_protocol_mode,
                                    std::string cert_path, std::string key_path, bool print_listen_info,
                                    std::uint32_t timeout, int max_request_count)
{
#ifdef _WIN32
    WSADATA wsaData;
    int initResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (initResult != 0)
    {
        throw std::runtime_error("WSAStartup failed");
    }
#endif

    if (http_port == DISABLE_HTTP_REQUEST && https_port == DISABLE_HTTPS_REQUEST)
    {
        throw std::runtime_error("At least one port must be enabled");
    }

    if (https_port != DISABLE_HTTPS_REQUEST)
    {
        // SSL 上下文
        if (!ssl_ctx_server)
        {
            HANDLE_ERROR("Unable to create SSL context");
            throw("Unable to create SSL context");
        }

        // 加载证书和私钥
        if (SSL_CTX_use_certificate_file(ssl_ctx_server, cert_path.data(), SSL_FILETYPE_PEM) <= 0)
        {
            HANDLE_ERROR("Unable to load certificate");
            throw("Unable to load certificate");
        }
        if (SSL_CTX_use_PrivateKey_file(ssl_ctx_server, key_path.data(), SSL_FILETYPE_PEM) <= 0)
        {
            HANDLE_ERROR("Unable to load private key");
            throw("Unable to load private key");
        }
        // 验证私钥是否与证书匹配
        if (!SSL_CTX_check_private_key(ssl_ctx_server))
        {
            HANDLE_ERROR("Private key does not match the certificate");
            throw("Private key does not match the certificate");
        }
    }

    SOCKET http_server_socket = INVALID_SOCKET;
    SOCKET https_server_socket = INVALID_SOCKET;

    // 根据 ip_protocol_mode 确定协议族和是否启用 IPv6 独占模式
    int ip_protocol_family = (ip_protocol_mode == 0) ? AF_INET : AF_INET6;
    bool is_ipv6_only = (ip_protocol_mode == 1);

    // --- 创建 HTTP 套接字 ---
    if (http_port != DISABLE_HTTP_REQUEST)
    {
        if (ip_protocol_mode == IP_PROTOCOL_MODE_IPV4_ONLY)
        { // 仅 IPv4
            http_server_socket = createAndBindSocket_Pimpl(http_port, AF_INET, false);
        }
        else
        { // IPv6 或双栈
            http_server_socket = createAndBindSocket_Pimpl(http_port, AF_INET6, is_ipv6_only);
        }
    }

    // --- 创建 HTTPS 套接字 ---
    if (https_port != DISABLE_HTTPS_REQUEST)
    {
        if (ip_protocol_mode == IP_PROTOCOL_MODE_IPV4_ONLY)
        { // 仅 IPv4
            https_server_socket = createAndBindSocket_Pimpl(https_port, AF_INET, false);
        }
        else
        { // IPv6 或双栈
            https_server_socket = createAndBindSocket_Pimpl(https_port, AF_INET6, is_ipv6_only);
        }
    }

    // 在创建 HTTP 套接字成功后
    if (http_server_socket != INVALID_SOCKET && print_listen_info)
    {
        std::string ip_version =
            (ip_protocol_mode == 0) ? "IPv4" : ((ip_protocol_mode == 1) ? "IPv6" : "IPv4 and IPv6");
        std::string ip_address =
            (ip_protocol_mode == 0) ? "0.0.0.0" : ((ip_protocol_mode == 1) ? "[::]" : "0.0.0.0 and [::]");
        std::cout << "Server listening on HTTP (" << ip_version << ") on " << ip_address << ":" << http_port
                  << std::endl;
    }

    // 在创建 HTTPS 套接字成功后
    if (https_server_socket != INVALID_SOCKET && print_listen_info)
    {
        std::string ip_version =
            (ip_protocol_mode == 0) ? "IPv4" : ((ip_protocol_mode == 1) ? "IPv6" : "IPv4 and IPv6");
        std::string ip_address =
            (ip_protocol_mode == 0) ? "0.0.0.0" : ((ip_protocol_mode == 1) ? "[::]" : "0.0.0.0 and [::]");
        std::cout << "Server listening on HTTPS (" << ip_version << ") on " << ip_address << ":" << https_port
                  << std::endl;
    }

    unsigned int cores = 0;
    if (http_server_socket != INVALID_SOCKET)
        cores++;
    if (https_server_socket != INVALID_SOCKET)
        cores++;
    if (cores <= 0)
    {
        log_e("Unable to get the number of CPU cores\n");
        throw("Unable to get the number of CPU cores");
    }
    threadPool accept_threadpool(cores); // 创建一个线程池

    try
    {
        // 将处理函数添加到线程池中
        if (http_server_socket != INVALID_SOCKET)
        {
            accept_threadpool.enqueue(
                [this, http_server_socket, http_port, https_port, behavior_mode, timeout, max_request_count]() {
                    this->acceptScocket_Pimpl(http_server_socket, false, http_port, https_port, behavior_mode, timeout,
                                              max_request_count);
                });
        }
        if (https_server_socket != INVALID_SOCKET)
        {
            accept_threadpool.enqueue(
                [this, https_server_socket, http_port, https_port, behavior_mode, timeout, max_request_count]() {
                    this->acceptScocket_Pimpl(https_server_socket, true, http_port, https_port, behavior_mode, timeout,
                                              max_request_count);
                });
        }
    }
    catch (const std::exception &e)
    {
        log_d("Sorry! Your program crashed. I caught this exception in the outermost function. This is the error it "
              "threw: %s\n",
              e.what());
    }
    catch (...)
    {
        log_e("Sorry! Your program crashed. I caught the exception in the outermost function, but could not identify "
              "the error thrown.");
    }
}

void cppNetworkUtilPimpl::acceptScocket_Pimpl(SOCKET server_socket, bool enable_https, int http_port, int https_port,
                                              int behavior_mode, std::uint32_t timeout, int max_request_count)
{
    if (server_socket == INVALID_SOCKET)
    {
        throw("Invalid server socket");
    }

    unsigned int cores = std::thread::hardware_concurrency();
    if (cores <= 0)
    {
        log_e("Unable to get the number of CPU cores\n");
        throw("Unable to get the number of CPU cores");
    }
    threadPool process_threadpool(cores * 3); // 创建一个线程池

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

        // 设置超时
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

        // 将处理函数添加到线程池中
        process_threadpool.enqueue([this, client_socket, client_address, ssl_conn, enable_https, http_port, https_port,
                                    behavior_mode, timeout, max_request_count]() {
            this->process(client_socket, client_address, ssl_conn, enable_https, http_port, https_port, behavior_mode,
                          timeout, max_request_count);
        });
    }
}

// 假设这是 cppNetworkUtilPimpl 类的成员函数实现
void cppNetworkUtilPimpl::process(SOCKET client_socket, struct sockaddr_storage client_address, ssl_st *ssl_conn,
                                  bool enable_https, int http_port, int https_port, int behavior_mode,
                                  std::uint32_t timeout, int max_request_count)
{
    std::string client_buffer;
    bool keep_running = true;
    int current_request_count = 0;

    // **注意：移除了局部 send_mutex 和 worker_pool。**
    // **SSL I/O (read/write) 现在是串行的，无需局部锁。**

    try
    {
        while (keep_running)
        {
            // 1) 读取并解析请求头（同步 I/O 操作）
            if (!ensureBufferUntil_Pimpl(client_socket, ssl_conn, client_buffer, "\r\n\r\n"))
            {
                // 连接关闭或读取错误
                break;
            }

            size_t header_end = client_buffer.find("\r\n\r\n");
            if (header_end == std::string::npos)
            {
                // 无效的请求
                break;
            }

            std::string header_str = client_buffer.substr(0, header_end);
            std::unordered_map<std::string, std::string> parsed_header;
            try
            {
                parsed_header = getParsedHeader_Pimpl(header_str);
            }
            catch (const std::exception &e)
            {
                log_e("Failed to parse request header: %s\n", e.what());
                // 返回 400 并断开
                std::unordered_map<std::string, std::string> err_headers;
                err_headers["Content-Length"] = "0";
                err_headers["connection"] = "close";
                // I/O 串行，直接发送
                sendDataToSocket_Pimpl(client_socket, current_request_count,
                                       makeResponseHeader_Pimpl(400, err_headers));
                break;
            }

            // 决定是否为 keep-alive
            bool is_keep_alive_connection = false;
            try
            {
                std::string connection = parsed_header.count("Connection") ? parsed_header["Connection"] : "";
                std::transform(connection.begin(), connection.end(), connection.begin(), ::tolower);
                if (connection == "keep-alive" || parsed_header["http_version"] == "HTTP/1.1")
                    is_keep_alive_connection = true;
            }
            catch (...)
            {
                // 忽略，按非 keep-alive 处理
            }

            // 移除请求行+头部
            client_buffer.erase(0, header_end + 4);

            // 处理 CONNECT 方法（HTTP proxy 隧道）
            std::string method = parsed_header.count("method") ? parsed_header["method"] : "";
            if (!enable_https && method == "CONNECT")
            {
                // URL 为 host:port
                std::string hostport = parsed_header.count("url") ? parsed_header["url"] : "";
                std::string host;
                int port = 443;
                size_t colon = hostport.find(':');
                if (colon != std::string::npos)
                {
                    host = hostport.substr(0, colon);
                    try
                    {
                        port = std::stoi(hostport.substr(colon + 1));
                    }
                    catch (...)
                    {
                        port = 443;
                    }
                }
                else
                {
                    host = hostport;
                }

                // 连接目标主机
                SOCKET remote_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (remote_sock == INVALID_SOCKET)
                {
                    log_e("Failed to create remote socket for CONNECT\n");
                    std::unordered_map<std::string, std::string> err_headers;
                    err_headers["Content-Length"] = "0";
                    err_headers["connection"] = "close";
                    sendDataToSocket_Pimpl(client_socket, current_request_count,
                                           makeResponseHeader_Pimpl(502, err_headers));
                    break;
                }

                struct hostent *remote_host = gethostbyname(host.c_str());
                if (!remote_host)
                {
                    closesocket(remote_sock);
                    std::unordered_map<std::string, std::string> err_headers;
                    err_headers["Content-Length"] = "0";
                    err_headers["connection"] = "close";
                    sendDataToSocket_Pimpl(client_socket, current_request_count,
                                           makeResponseHeader_Pimpl(502, err_headers));
                    break;
                }

                struct sockaddr_in remote_addr;
                memset(&remote_addr, 0, sizeof(remote_addr));
                remote_addr.sin_family = AF_INET;
                remote_addr.sin_port = htons(port);
                memcpy(&remote_addr.sin_addr, remote_host->h_addr_list[0], remote_host->h_length);

                if (connect(remote_sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) == SOCKET_ERROR)
                {
                    HANDLE_ERROR("Failed to connect remote for CONNECT");
                    closesocket(remote_sock);
                    std::unordered_map<std::string, std::string> err_headers;
                    err_headers["Content-Length"] = "0";
                    err_headers["connection"] = "close";
                    sendDataToSocket_Pimpl(client_socket, current_request_count,
                                           makeResponseHeader_Pimpl(502, err_headers));
                    break;
                }

                // 回复客户端 200 Connection Established
                std::unordered_map<std::string, std::string> ok_headers;
                ok_headers["Content-Length"] = "0";
                ok_headers["connection"] = "close";
                sendDataToSocket_Pimpl(client_socket, current_request_count, makeResponseHeader_Pimpl(200, ok_headers));

                // 双向隧道转发（阻塞直到一方关闭）
                fd_set readfds;
                int maxfd = (int)std::max(client_socket, remote_sock) + 1;
                while (true)
                {
                    FD_ZERO(&readfds);
                    FD_SET(client_socket, &readfds);
                    FD_SET(remote_sock, &readfds);
                    int sel = select(maxfd, &readfds, NULL, NULL, NULL);
                    if (sel <= 0)
                        break;
                    if (FD_ISSET(client_socket, &readfds))
                    {
                        char buf[4096];
                        int r = recv(client_socket, buf, sizeof(buf), 0);
                        if (r <= 0)
                            break;
                        int sent = 0;
                        while (sent < r)
                        {
                            int s = send(remote_sock, buf + sent, r - sent, 0);
                            if (s <= 0)
                                break;
                            sent += s;
                        }
                        if (sent < r)
                            break;
                    }
                    if (FD_ISSET(remote_sock, &readfds))
                    {
                        char buf[4096];
                        int r = recv(remote_sock, buf, sizeof(buf), 0);
                        if (r <= 0)
                            break;
                        int sent = 0;
                        while (sent < r)
                        {
                            int s = send(client_socket, buf + sent, r - sent, 0);
                            if (s <= 0)
                                break;
                            sent += s;
                        }
                        if (sent < r)
                            break;
                    }
                }
                closesocket(remote_sock);
                keep_running = false;
                break;
            }

            // 2) 读取 Body（支持 chunked 与 content-length）
            std::string request_body;
            bool body_ok = true;
            std::string transfer_encoding;
            try
            {
                transfer_encoding = parsed_header.at("Transfer-Encoding");
            }
            catch (...)
            {
                log_e("Get Transfer-Encoding from parsed request header failed.\n");
            }
            std::transform(transfer_encoding.begin(), transfer_encoding.end(), transfer_encoding.begin(), ::tolower);
            if (transfer_encoding == "chunked")
            {
                body_ok = parseChunkedBody_Pimpl(client_socket, ssl_conn, client_buffer, request_body);
            }
            else
            {
                std::string content_length_str;
                try
                {
                    content_length_str = parsed_header.at("Content-Length");
                }
                catch (...)
                {
                    log_e("Get Content-Length from parsed request header failed.\n");
                }
                long content_length = -1;
                if (!content_length_str.empty())
                {
                    try
                    {
                        content_length = std::stol(content_length_str);
                    }
                    catch (...)
                    {
                        content_length = -1;
                    }
                }

                if (content_length > 0)
                {
                    // 确保缓冲区包含整个请求体
                    if (!ensureBuffer_Pimpl(client_socket, ssl_conn, client_buffer,
                                            static_cast<size_t>(content_length)))
                    {
                        body_ok = false;
                    }
                    else
                    {
                        request_body = client_buffer.substr(0, content_length);
                        client_buffer.erase(0, content_length);
                    }
                }
                else
                {
                    // content_length <= 0 : 视为无 body（GET/HEAD 等）
                    request_body.clear();
                }
            }

            if (!body_ok)
            {
                log_e("Failed to read request body for socket %d\n", (int)client_socket);
                break;
            }

            // 增加请求计数（用于 keep-alive 限制）
            if (is_keep_alive_connection)
                ++current_request_count;

            // 在某些行为模式下：HTTP 重定向到 HTTPS
            if (!enable_https && behavior_mode == BEHAVIOR_MODE_REDIRECT_HTTP_REQUEST_TO_HTTPS &&
                https_port != DISABLE_HTTPS_REQUEST)
            {
                std::string Location = "https://";
                Location += parsed_header.count("Host") ? parsed_header["Host"] : "";
                Location += parsed_header.count("url") ? parsed_header["url"] : "";
                if (https_port != 443)
                {
                    Location += ":" + std::to_string(https_port);
                }

                std::unordered_map<std::string, std::string> resp_headers;
                resp_headers["Content-Length"] = "0";
                if (is_keep_alive_connection && current_request_count < max_request_count)
                {
                    resp_headers["connection"] = "keep-alive";
                    resp_headers["Keep-Alive"] = "timeout=" + std::to_string(timeout / 1000) +
                                                 ", max=" + std::to_string(max_request_count - current_request_count);
                    resp_headers["Location"] = Location;
                }
                else
                {
                    resp_headers["connection"] = "close";
                    resp_headers["Location"] = Location;
                    keep_running = false;
                }
                // I/O 串行，直接发送
                sendDataToSocket_Pimpl(client_socket, current_request_count,
                                       makeResponseHeader_Pimpl(301, resp_headers));
                break; // 在重定向后关闭连接
            }

            // **修改点 1: 保护 client_connections 写入**
            // 将请求信息放入 client_connections（便于 send 使用）
            {
                std::lock_guard<std::mutex> lg(this->client_connections_mutex); // 使用成员互斥锁

                client_connections[client_socket][current_request_count].ssl = ssl_conn;
                client_connections[client_socket][current_request_count].is_https_connection = enable_https;
                client_connections[client_socket][current_request_count].is_keep_alive_connection =
                    is_keep_alive_connection;

                if (client_address.ss_family == AF_INET)
                {
                    struct sockaddr_in *ipv4_addr = (struct sockaddr_in *)&client_address;
                    char ip_str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(ipv4_addr->sin_addr), ip_str, INET_ADDRSTRLEN);
                    client_connections[client_socket][current_request_count].ip = ip_str;
                    client_connections[client_socket][current_request_count].port = ntohs(ipv4_addr->sin_port);
                    client_connections[client_socket][current_request_count].family = "ipv4";
                }
                else if (client_address.ss_family == AF_INET6)
                {
                    struct sockaddr_in6 *ipv6_addr = (struct sockaddr_in6 *)&client_address;
                    char ip_str[INET6_ADDRSTRLEN];
                    inet_ntop(AF_INET6, &(ipv6_addr->sin6_addr), ip_str, INET6_ADDRSTRLEN);
                    client_connections[client_socket][current_request_count].ip = ip_str;
                    client_connections[client_socket][current_request_count].port = ntohs(ipv6_addr->sin6_port);
                    client_connections[client_socket][current_request_count].family = "ipv6";
                }

                client_connections[client_socket][current_request_count].request_data = client_buffer;
                client_connections[client_socket][current_request_count].request_header = header_str;
                client_connections[client_socket][current_request_count].request_content = request_body;
                client_connections[client_socket][current_request_count].parsed_request_headers = parsed_header;
                client_connections[client_socket][current_request_count].query_params =
                    parseUrlQueryParameters_Pimpl(parsed_header["url"]);

                // 如果 URL 包含查询字符串，移除之以作为 path
                {
                    std::string url =
                        client_connections[client_socket][current_request_count].parsed_request_headers["url"];
                    size_t q = url.find('?');
                    if (q != std::string::npos)
                        client_connections[client_socket][current_request_count].parsed_request_headers["url"] =
                            url.substr(0, q);
                }
            } // client_connections_mutex 释放

            // 3) 构造 requestContext 并 **同步处理**
            requestContext req;
            req.client_socket = client_socket;
            req.request_count = current_request_count;
            req.is_https_connection = enable_https;
            req.is_keep_alive_connection = is_keep_alive_connection;

            // 从 client_connections 复制信息（已在锁内设置）
            {
                std::lock_guard<std::mutex> lg(this->client_connections_mutex);
                req.ip = client_connections[client_socket][current_request_count].ip;
                req.port = client_connections[client_socket][current_request_count].port;
                req.family = client_connections[client_socket][current_request_count].family;
                req.request_data = client_connections[client_socket][current_request_count].request_data;
                req.request_content = client_connections[client_socket][current_request_count].request_content;
                req.request_headers = client_connections[client_socket][current_request_count].request_header;
                req.parsed_request_headers =
                    client_connections[client_socket][current_request_count].parsed_request_headers;
                req.query_params = client_connections[client_socket][current_request_count].query_params;
            }

            // **同步调用处理函数** (替代 enqueue 到 worker_pool)
            std::optional<responseContext> res = handleRequest_Pimpl(req);

            // 4) 发送响应 (同步 I/O 操作)
            if (res.has_value())
            {
                // 设置连接头和 Content-Length
                if (is_keep_alive_connection && current_request_count < max_request_count)
                {
                    res->response_headers["connection"] = "keep-alive";
                    res->response_headers["Keep-Alive"] = "timeout=" + std::to_string(timeout / 1000) + ", max=" +
                                                          std::to_string(max_request_count - current_request_count);
                }
                else
                {
                    res->response_headers["connection"] = "close";
                    keep_running = false; // 非 keep-alive 或达到限制，准备退出循环
                }
                res->response_headers["Content-Length"] = std::to_string(res->response_content.size());

                std::string header_to_send = makeResponseHeader_Pimpl(res->status_code, res->response_headers);

                // I/O 串行，直接发送
                sendDataToSocket_Pimpl(client_socket, current_request_count, header_to_send);
                if (!res->response_content.empty())
                    sendDataToSocket_Pimpl(client_socket, current_request_count, res->response_content);
            }
            else
            {
                // PROCESSED_INTERNALLY
            }

            // **修改点 2: 保护 client_connections 清理**
            // 清理 client_connections 中该请求的数据（使用成员互斥锁保护）
            {
                std::lock_guard<std::mutex> lg(this->client_connections_mutex);
                auto it_outer = this->client_connections.find(client_socket);
                if (it_outer != this->client_connections.end())
                {
                    it_outer->second.erase(current_request_count);
                    if (it_outer->second.empty())
                        this->client_connections.erase(it_outer);
                }
            }

            // 检查是否应该继续（keep-alive 和请求计数）
            if (!is_keep_alive_connection || current_request_count >= max_request_count)
            {
                keep_running = false;
            }
        } // end while keep_running
    }
    catch (const std::exception &e)
    {
        log_e("Connection loop exception: %s\n", e.what());
    }
    catch (...)
    {
        log_e("Unknown exception in connection loop\n");
    }

    // 清理 SSL / socket
    if (enable_https && ssl_conn)
    {
        // 尝试执行 SSL 关闭握手
        SSL_shutdown(ssl_conn);
        SSL_free(ssl_conn);
    }

    closesocket(client_socket);

    // **修改点 3: 保护 client_connections 最终清理**
    // 最终清理 client_connections（使用成员互斥锁保护）
    {
        std::lock_guard<std::mutex> lg(this->client_connections_mutex);
        this->client_connections.erase(client_socket);
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

bool cppNetworkUtilPimpl::containsTemplate(const std::string &pattern)
{
    return pattern.find('{') != std::string::npos && pattern.find('}') != std::string::npos;
}

bool cppNetworkUtilPimpl::isPureRegex(const std::string &pattern)
{
    return (pattern.find_first_of("^$|?*+()[]") != std::string::npos) && !containsTemplate(pattern);
}

std::string cppNetworkUtilPimpl::parseAndCompileTemplate(const std::string &template_path,
                                                         std::vector<std::string> &param_names)
{
    std::string regex_str = "^"; // 确保以开始锚点 ^ 开头
    size_t current = 0;

    // 用于转义固定路径部分中的正则表达式元字符
    auto escape_fixed_part = [](const std::string &part) {
        std::string escaped_part = part;
        // 转义所有正则表达式元字符，特别是 . / ? * +
        std::regex fixed_regex_chars("[.^$|?*+()\\/]");
        return std::regex_replace(escaped_part, fixed_regex_chars, std::string("\\$&"));
    };

    while (current < template_path.length())
    {
        size_t start = template_path.find('{', current);

        // 1. 处理固定路径部分 (从 current 到 start)
        if (start == std::string::npos)
        {
            // 如果找不到模板变量，则将剩余部分作为固定路径处理
            regex_str += escape_fixed_part(template_path.substr(current));
            break;
        }
        else
        {
            // 将 { 之前的部分作为固定路径，并转义
            regex_str += escape_fixed_part(template_path.substr(current, start - current));
        }

        // 2. 找到模板变量的结束 }
        size_t end = template_path.find('}', start);
        if (end == std::string::npos)
        {
            // 模板格式错误，缺少 '}'
            throw std::runtime_error("Template error: Missing closing brace '}'");
        }

        // 3. 提取模板内容 (name:regex)
        std::string var_content = template_path.substr(start + 1, end - start - 1);
        size_t colon_pos = var_content.find(':');

        std::string param_name;
        std::string param_regex;

        // 4. 解析 name 和 regex
        if (colon_pos == std::string::npos)
        {
            // 如果没有指定正则表达式，使用默认的通配符 ([^/]+)
            param_name = var_content;
            param_regex = "([^/]+)"; // 默认匹配非斜杠的任意字符
        }
        else
        {
            // 提取变量名和自定义正则表达式
            param_name = var_content.substr(0, colon_pos);
            // 仅提取正则表达式部分
            param_regex = var_content.substr(colon_pos + 1);

            // 将用户提供的 regex 封装在捕获组中
            param_regex = "(" + param_regex + ")";
        }

        // 5. 存储参数名并添加到最终正则表达式
        if (param_name.empty())
        {
            throw std::runtime_error("Template error: Variable name cannot be empty");
        }
        param_names.push_back(param_name);
        regex_str += param_regex;

        // 6. 更新当前位置，继续处理模板字符串的剩余部分
        current = end + 1;
    }

    regex_str += "$"; // 确保以结束锚点 $ 结尾
    return regex_str;
}

std::string cppNetworkUtilPimpl::makeRagexString_Pimpl(const std::string &path_pattern,
                                                       std::vector<std::string> &param_names)
{
    std::string regex_str;

    if (containsTemplate(path_pattern))
    {
        regex_str = parseAndCompileTemplate(path_pattern, param_names);

        // std::cout << "Register TEMPLATE RouteHandler: " << " " << path_pattern
        //           << " -> to: " << final_regex_str << "\n";
    }
    else if (isPureRegex(path_pattern))
    {
        regex_str = path_pattern;

        // 确保包含 ^ 和 $，以防止 std::regex_match 陷入困境
        if (regex_str.front() != '^')
        {
            regex_str.insert(0, "^");
        }
        if (regex_str.back() != '$')
        {
            regex_str.append("$");
        }

        // std::cout << "Register PURE REGEX RouteHandler: " << " " << path_pattern
        //           << " -> to: " << regex_str << "\n";
    }
    else
    {
        regex_str = path_pattern;
        std::regex fixed_regex_chars("[.^$|?*+()\\/]");
        regex_str = std::regex_replace(regex_str, fixed_regex_chars, std::string("\\$&"));
        regex_str = "^" + regex_str + "$";

        // std::cout << "Register FIXED RouteHandler: " << " " << path_pattern << " -> to: " <<
        // regex_str << "\n";
    }

    return regex_str;
}

void cppNetworkUtilPimpl::on_Pimpl(const std::string &method, const int &status_code, const std::string &path_pattern,
                                   const routeHandler &handler)
{
    routeInfo info;
    info.path_pattern = path_pattern;
    info.status_code = status_code;
    info.handler = std::move(handler);

    try
    {
        // 构造正则表达式字符串并写入路由信息结构体
        info.method_regex = std::regex((method == "*") ? ".*" : method, REGEX_FLAGS);

        info.path_regex = std::regex(makeRagexString_Pimpl(path_pattern, info.param_names), REGEX_FLAGS);

        // 存入路由表
        handlers[method].push_back(std::move(info));

        // std::cout << "Registered RouteHandler: [" << method << "] "
        //           << "(" << status_code << ") " << path_pattern << "\n";
    }
    catch (const std::regex_error &e)
    {
        log_e("Regex Error for pattern '%s': %s\n", path_pattern.data(), e.what());
        return;
    }
}

void cppNetworkUtilPimpl::off_Pimpl(const std::string &method, const int &status_code, const std::string &path_pattern)
{
    if (isRegexPattern_Pimpl(method))
    {
        auto method_it = handlers.begin();
        while (method_it != handlers.end())
        {
            // 找到匹配 method 的 entry
            if (!method_it->second.empty() && method_it->second[0].path_pattern == method)
            {
                std::vector<routeInfo> &routes = method_it->second;

                // 移除匹配的路径模式
                auto new_end =
                    std::remove_if(routes.begin(), routes.end(), [&path_pattern, &status_code](const routeInfo &info) {
                        return info.path_pattern == path_pattern && info.status_code == status_code;
                    });

                routes.erase(new_end, routes.end());

                if (routes.empty())
                {
                    method_it = handlers.erase(method_it);
                }
                else
                {
                    ++method_it;
                }
                return;
            }
            else
            {
                ++method_it;
            }
        }
    }
    else
    {
        auto method_it = handlers.find(method);
        if (method_it != handlers.end())
        {
            std::vector<routeInfo> &routes = method_it->second;

            // 移除路径模式完全匹配的路由
            auto new_end =
                std::remove_if(routes.begin(), routes.end(), [&path_pattern, &status_code](const routeInfo &info) {
                    return info.path_pattern == path_pattern && info.status_code == status_code;
                });

            routes.erase(new_end, routes.end());

            if (routes.empty())
            {
                handlers.erase(method_it);
            }
            return;
        }
    }
}

void cppNetworkUtilPimpl::invokeErrorHandler_Pimpl(int status_code, const requestContext &req, responseContext &res)
{
    res.status_code = status_code;
    res.response_headers["status_code"] = std::to_string(res.status_code);

    const std::string &method = req.parsed_request_headers.at("method");
    const std::string &url = req.parsed_request_headers.at("url");

    // 使用基于迭代器的 match_results 避免 std::smatch 的额外分配
    auto find_and_handle_path = [&](const std::vector<routeInfo> &routes_to_check) -> std::optional<int> {
        std::match_results<std::string::const_iterator> match_results;
        for (const routeInfo &route_info : routes_to_check)
        {
            // 快速过滤：状态码不匹配或路径模式不以 '/' 或 '^' 开头则跳过
            if (route_info.status_code != status_code)
                continue;
            const std::string &pattern = route_info.path_pattern;
            if (pattern.empty() || (pattern[0] != '/' && pattern[0] != '^'))
                continue;

            try
            {
                // 使用迭代器版本的 regex_match 避免不必要的字符串临时对象
                if (!std::regex_match(url.begin(), url.end(), match_results, route_info.path_regex))
                    continue;

                // 仅在匹配成功时构造 matched_req（避免不必要的拷贝）
                requestContext matched_req = req;

                // 有效参数数量 = match_results.size() - 1 (0 是完整匹配)
                size_t available = match_results.size() ? (match_results.size() - 1) : 0;
                size_t to_copy = std::min(route_info.param_names.size(), available);
                for (size_t i = 0; i < to_copy; ++i)
                {
                    matched_req.path_params[route_info.param_names[i]] = match_results[i + 1].str();
                }

                int response_is_final = route_info.handler(matched_req, res);

                // 与原逻辑保持一致：这些返回值意味着处理结束或特殊处理
                if (response_is_final == END_HANDING || response_is_final == PROCESSED_INTERNALLY ||
                    response_is_final == CONTINUE_HANDLING)
                {
                    return response_is_final;
                }
                else if (response_is_final == CONTINUE_ROUTING)
                {
                    continue;
                }
            }
            catch (const std::regex_error &e)
            {
                log_e("Regex error while matching pattern '%s': %s\n", pattern.c_str(), e.what());
                continue;
            }
            catch (const std::exception &e)
            {
                log_e("Handler/Match exception for pattern '%s': %s\n", pattern.c_str(), e.what());
                continue;
            }
            catch (...)
            {
                log_e("Unknown exception while processing pattern '%s'\n", pattern.c_str());
                continue;
            }
        }
        return std::nullopt;
    };

    // 优先尝试完全匹配的 method 分组（快速路径）
    {
        auto method_it = handlers.find(method);
        if (method_it != handlers.end())
        {
            if (auto r = find_and_handle_path(method_it->second); r.has_value())
            {
                int r_value = r.value();
                if (r_value == END_HANDING || r_value == CONTINUE_HANDLING || r_value == PROCESSED_INTERNALLY)
                    return;
            }
        }
    }

    // 如果 exact method 未命中，尝试带 method 正则的分组（避免对每个 route 重复构造 method regex）
    for (const auto &entry : handlers)
    {
        if (entry.first == method)
            continue;
        const std::vector<routeInfo> &routes = entry.second;
        if (routes.empty())
            continue;

        try
        {
            const std::regex &method_re = routes[0].method_regex;
            if (!std::regex_match(method, method_re))
                continue;

            if (auto r = find_and_handle_path(routes); r.has_value())
            {
                int r_value = r.value();
                if (r_value == END_HANDING || r_value == CONTINUE_HANDLING || r_value == PROCESSED_INTERNALLY)
                    return;
            }
        }
        catch (const std::regex_error &e)
        {
            log_e("Method regex error while matching request method '%s': %s\n", method.data(), e.what());
            continue;
        }
    }

    // 回退到默认处理（保持原有语义）
    if (status_code < 100 || status_code > 599)
    {
        status_code = 500; // 非法状态码，回退到 500
    }
    else if (status_code / 100 == 3)
    {
        // 3xx 重定向类状态码，默认不处理
        return;
    }
    res.response_content = std::to_string(status_code) + " " + getHttpCodeText_Pimpl(status_code).value_or("Unknown");
    res.response_headers["Content-Type"] = "text/plain";
}

std::optional<responseContext> cppNetworkUtilPimpl::handleRequest_Pimpl(const requestContext &req)
{
    responseContext res;
    res.response_headers["Content-Type"] = "*/*";
    res.status_code = 200;
    res.response_headers["connection"] = "keep-alive";
    if (req.is_https_connection)
        res.response_headers["Strict-Transport-Security"] = "max-age=31536000; includeSubDomains; preload";

    const std::string &method = req.parsed_request_headers.at("method");
    const std::string &url = req.parsed_request_headers.at("url");

    auto find_and_handle_path = [&](const std::vector<routeInfo> &routes_to_check) -> std::optional<int> {
        // reuse match_results object to avoid repeated allocations
        std::match_results<std::string::const_iterator> match_results;
        for (const routeInfo &route_info : routes_to_check)
        {
            const std::string &pattern = route_info.path_pattern;
            const int route_status = route_info.status_code;

            if (pattern.empty() || (pattern[0] != '/' && pattern[0] != '^'))
                continue;

            // keep original logic but expressed clearer and faster
            if (route_status >= 400 && route_status <= 600)
                continue;

            try
            {
                // perform regex match using iterators (avoids extra string temporaries)
                if (!std::regex_match(url.begin(), url.end(), match_results, route_info.path_regex))
                    continue;

                // construct matched_req only after a successful match
                requestContext matched_req = req;
                const size_t param_count = route_info.param_names.size();
                size_t available = match_results.size() > 0 ? (match_results.size() - 1) : 0;
                size_t to_copy = std::min(param_count, available);
                for (size_t i = 0; i < to_copy; ++i)
                {
                    matched_req.path_params[route_info.param_names[i]] = match_results[i + 1].str();
                }

                int response_is_final = route_info.handler(matched_req, res);

                if (response_is_final == END_HANDING || response_is_final == PROCESSED_INTERNALLY)
                    return response_is_final;

                if (response_is_final == CONTINUE_ROUTING)
                    continue;

                if (res.status_code >= 300 && res.status_code < 600)
                    invokeErrorHandler_Pimpl(res.status_code, matched_req, res);

                return END_HANDING;
            }
            catch (const std::regex_error &e)
            {
                log_e("Regex error while matching pattern '%s': %s\n", pattern.c_str(), e.what());
                continue;
            }
            catch (const std::exception &e)
            {
                log_e("Handler/Match exception for pattern '%s': %s\n", pattern.c_str(), e.what());
                continue;
            }
            catch (...)
            {
                log_e("Unknown exception while processing pattern '%s'\n", pattern.c_str());
                continue;
            }
        }
        return std::nullopt;
    };

    // fast path: exact method handlers
    {
        auto it = handlers.find(method);
        if (it != handlers.end())
        {
            if (auto r = find_and_handle_path(it->second); r.has_value())
            {
                int v = r.value();
                if (v == END_HANDING || v == CONTINUE_HANDLING)
                    return res;
                if (v == PROCESSED_INTERNALLY)
                    return std::nullopt;
            }
        }
    }

    // method-pattern handlers: iterate entries but avoid constructing regex repeatedly
    for (const auto &entry : handlers)
    {
        if (entry.first == method)
            continue;

        const std::vector<routeInfo> &routes = entry.second;
        if (routes.empty())
            continue;

        try
        {
            const std::regex &method_re = routes[0].method_regex;
            if (!std::regex_match(method, method_re))
                continue;

            if (auto r = find_and_handle_path(routes); r.has_value())
            {
                int v = r.value();
                if (v == END_HANDING || v == CONTINUE_HANDLING)
                    return res;
                if (v == PROCESSED_INTERNALLY)
                    return std::nullopt;
            }
        }
        catch (const std::regex_error &e)
        {
            log_e("Method regex error while matching request method '%s': %s\n", method.data(), e.what());
            continue;
        }
    }

    // fallback 404
    invokeErrorHandler_Pimpl(404, req, res);
    return res;
}
