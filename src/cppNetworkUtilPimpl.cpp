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

    if (parameters.count("connection"))
    {
        std::optional<std::string> title = getHttpCodeText_Pimpl(status_code);

        buffer += PROTOCOL;
        buffer += " ";
        buffer += std::to_string(status_code);
        buffer += " ";
        buffer += title.value_or("Unknown");
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
        buffer += "Connection: close\r\n";
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
std::unordered_map<std::string, multipartData> cppNetworkUtilPimpl::parseMultipart_Pimpl(const std::string &boundary,
                                                                                         const std::string &body)
{
    std::unordered_map<std::string, multipartData> parsedParts; // 存储所有解析出的部分
    std::string delimiter = "--" + boundary;                    // 每个部分的开始分隔符
    std::string endDelimiter = delimiter + "--";                // 整个 multipart 结束的分隔符
    size_t pos = 0;                                             // 当前在 body 字符串中的查找位置

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
        transform(headers_lower.begin(), headers_lower.end(), headers_lower.begin(), ::toupper); // 提取头部字符串
        std::string data = part.substr(
            headersEnd + (part.find("\r\n\r\n") != std::string::npos ? 4 : 2)); // 提取数据字符串，跳过分隔符长度

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

void cppNetworkUtilPimpl::sendDataToHttpsSocket_Pimpl(ssl_st *ssl, const std::string &data)
{
    SSL_write(ssl, data.data(), data.length()); // 发送数据到 SSL 套接字
}

void cppNetworkUtilPimpl::sendDataToSocket_Pimpl(SOCKET socket, const std::string &data)
{
    if (client_connections[socket].is_https_connection)
    {
        sendDataToHttpsSocket_Pimpl(socket, data);
    }
    else
    {
        sendDataToHttpSocket_Pimpl(socket, data);
    }
}

void cppNetworkUtilPimpl::sendDataChunkToSocket_Pimpl(SOCKET socket, const std::string &data)
{
    size_t chunk_size = data.length(); // 数据块大小

    std::stringstream chunk_size_stream;                  // 创建字符串流
    chunk_size_stream << std::hex << chunk_size;          // 转换为十六进制字符串
    std::string chunk_size_str = chunk_size_stream.str(); // 获取字符串

    sendDataToSocket_Pimpl(socket, chunk_size_str); // 发送块大小
    sendDataToSocket_Pimpl(socket, "\r\n");         // 发送 CRLF

    sendDataToSocket_Pimpl(socket, data);   // 发送数据块
    sendDataToSocket_Pimpl(socket, "\r\n"); // 发送 CRLF
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
                                    std::string cert_path, std::string key_path, bool print_listen_info)
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
    threadPool threadPool(cores); // 创建一个线程池

    try
    {
        // 将处理函数添加到线程池中
        if (http_server_socket != INVALID_SOCKET)
        {
            threadPool.enqueue([this, http_server_socket, http_port, https_port, behavior_mode]() {
                this->acceptScocket_Pimpl(http_server_socket, false, http_port, https_port, behavior_mode);
            });
        }
        if (https_server_socket != INVALID_SOCKET)
        {
            threadPool.enqueue([this, https_server_socket, http_port, https_port, behavior_mode]() {
                this->acceptScocket_Pimpl(https_server_socket, true, http_port, https_port, behavior_mode);
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
                                              int behavior_mode)
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
    threadPool threadPool(cores * 3); // 创建一个线程池

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

        // 将处理函数添加到线程池中
        threadPool.enqueue([this, server_socket, client_socket, client_address, ssl_conn, enable_https, http_port,
                            https_port, behavior_mode]() {
            this->process(server_socket, client_socket, client_address, ssl_conn, enable_https, http_port, https_port,
                          behavior_mode);
        });
    }
}

void cppNetworkUtilPimpl::process(SOCKET server_socket, SOCKET client_socket, struct sockaddr_storage client_address,
                                  ssl_st *ssl_conn, bool enable_https, int http_port, int https_port, int behavior_mode)
{
    if (server_socket == INVALID_SOCKET)
    {
        throw("Invalid server socket");
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
        return; // 继续等待下一个连接
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
        return;                     // 继续等待下一个连接
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

    if (!enable_https && behavior_mode == BEHAVIOR_MODE_REDIRECT_HTTP_REQUEST_TO_HTTPS &&
        https_port != DISABLE_HTTPS_REQUEST)
    {
        std::unordered_map<std::string, std::string> parsed_header = getParsedHeader_Pimpl(request_header);

        std::string Location;
        Location += "https://";
        Location += parsed_header["Host"];
        Location += parsed_header["url"];
        if (https_port != 443)
        {
            Location += ":";
            Location += std::to_string(https_port);
        }
        // log_d("Location=%s\n", Location.data());
        // std::cout << "Location=" << Location << "\n";
        sendDataToHttpSocket_Pimpl(client_socket,
                                   makeResponseHeader_Pimpl(301, {{"connection", "close"}, {"Location", Location}}));
        closesocket(client_socket);
        return;
    }

    if (enable_https)
    {
        client_connections[client_socket].ssl = ssl_conn; // 将 SSL 结构存储到 client_connections 中
    }
    client_connections[client_socket].is_https_connection = enable_https;
    if (client_address.ss_family == AF_INET)
    {
        // IPv4 连接
        struct sockaddr_in *ipv4_addr = (struct sockaddr_in *)&client_address;
        inet_ntop(AF_INET, &(ipv4_addr->sin_addr), client_connections[client_socket].ip.data(),
                  client_connections[client_socket].ip.length());
        client_connections[client_socket].port = ntohs(ipv4_addr->sin_port);
        client_connections[client_socket].family = "ipv4";
    }
    else if (client_address.ss_family == AF_INET6)
    {
        // IPv6 连接
        struct sockaddr_in6 *ipv6_addr = (struct sockaddr_in6 *)&client_address;
        inet_ntop(AF_INET6, &(ipv6_addr->sin6_addr), client_connections[client_socket].ip.data(),
                  client_connections[client_socket].ip.length());
        client_connections[client_socket].port = ntohs(ipv6_addr->sin6_port);
        client_connections[client_socket].family = "ipv6";
    }
    client_connections[client_socket].request_data = request_data; // 将接收到的数据存储到 client_connections 中
    client_connections[client_socket].request_header = request_header;
    client_connections[client_socket].request_content = request_content;
    client_connections[client_socket].parsed_request_headers = getParsedHeader_Pimpl(request_header); // 解析请求头
    client_connections[client_socket].query_params =
        parseUrlQueryParameters_Pimpl(client_connections[client_socket].parsed_request_headers["url"]);

    std::string url = client_connections[client_socket].parsed_request_headers["url"];
    size_t query_pos = url.find('?');
    if (query_pos != std::string::npos)
    {
        // 找到了 '?'，分割 URL 路径和查询参数
        client_connections[client_socket].parsed_request_headers["url"] = url.substr(0, query_pos);
    }
    else
    {
        // 没有查询参数，整个 URL 都是路径
        client_connections[client_socket].parsed_request_headers["url"] = url;
    }

    requestContext req;
    req.client_socket = client_socket;
    req.is_https_connection = client_connections[client_socket].is_https_connection;
    req.ip = client_connections[client_socket].ip;
    req.port = client_connections[client_socket].port;
    req.family = client_connections[client_socket].family;
    req.request_data = client_connections[client_socket].request_data;
    req.request_content = client_connections[client_socket].request_content;
    req.request_headers = client_connections[client_socket].request_header;
    req.parsed_request_headers = client_connections[client_socket].parsed_request_headers;
    req.query_params = client_connections[client_socket].query_params;

    std::optional<responseContext> res = handleRequest_Pimpl(req);
    if (res.has_value())
    {
        // send data to client
        sendDataToSocket_Pimpl(client_socket,
                               makeResponseHeader_Pimpl(res.value().status_code, res.value().response_headers));
        sendDataToSocket_Pimpl(client_socket, res.value().response_content);
    }

    if (enable_https)
    {
        SSL_shutdown(ssl_conn); // 尝试执行 SSL 关闭握手
        SSL_free(ssl_conn);     // 释放 SSL 结构
    }
    closesocket(client_socket); // 关闭套接字
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

void cppNetworkUtilPimpl::saveHandler_Pimpl(const std::string &method, const routeInfo &info)
{
    if (isRegexPattern_Pimpl(method))
    {
        try
        {
            std::string method_regex_str = (method == "*") ? ".*" : method;
            regex_method_handlers.push_back({std::regex(method_regex_str, REGEX_FLAGS), {std::move(info)}});
        }
        catch (const std::regex_error &e)
        {
            log_e("Regex Error for pattern '%s': %s\n", method.data(), e.what());
            throw;
        }
    }
    else
    {
        fixed_method_handlers[method].push_back(std::move(info));
    }
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
        info.path_regex = std::regex(makeRagexString_Pimpl(path_pattern, info.param_names), REGEX_FLAGS);

        // 存入主路由地图 (保持不变)
        saveHandler_Pimpl(method, std::move(info));
    }
    catch (const std::regex_error &e)
    {
        log_e("Regex Error for pattern '%s': %s\n", path_pattern.data(), e.what());
        return;
    }
}

void cppNetworkUtilPimpl::off_Pimpl(const std::string &method, const int &status_code, const std::string &path_pattern)
{
    // 1. 检查是否为固定方法路由
    auto fixed_it = fixed_method_handlers.find(method);
    if (fixed_it != fixed_method_handlers.end())
    {
        std::vector<routeInfo> &routes = fixed_it->second;

        // 移除路径模式完全匹配的路由
        auto new_end =
            std::remove_if(routes.begin(), routes.end(), [&path_pattern, &status_code](const routeInfo &info) {
                return info.path_pattern == path_pattern && info.status_code == status_code;
            });

        routes.erase(new_end, routes.end());

        if (routes.empty())
        {
            fixed_method_handlers.erase(fixed_it);
        }
        return;
    }

    // 2. 检查是否为正则表达式方法路由
    if (isRegexPattern_Pimpl(method))
    {
        auto method_it = regex_method_handlers.begin();
        while (method_it != regex_method_handlers.end())
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
                    method_it = regex_method_handlers.erase(method_it);
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
}

void cppNetworkUtilPimpl::invokeErrorHandler_Pimpl(int status_code, const requestContext &req, responseContext &res)
{
    const std::string &method = req.parsed_request_headers.at("method");
    const std::string &url = req.parsed_request_headers.at("url");

    auto find_and_handle_path = [&](const std::vector<routeInfo> &routes_to_check) -> std::optional<int> {
        for (const auto &route_info : routes_to_check)
        {
            // 允许以 '/' 开头 (固定/模板路径) 或以 '^' 开头 (纯正则表达式路径)
            const std::string &pattern = route_info.path_pattern;

            // 检查路径模式是否以 '/' 或 '^' 开头
            if (!pattern.empty() && (pattern[0] == '/' || pattern[0] == '^') && route_info.status_code == status_code)
            {
                std::smatch match_results;
                // std::cout << "Trying to match URL: " << url << " with pattern: " << pattern << "\n";
                if (std::regex_match(url, match_results, route_info.path_regex))
                {
                    requestContext matched_req = req;
                    for (size_t i = 0; i < route_info.param_names.size(); ++i)
                    {
                        if (i + 1 < match_results.size())
                        {
                            matched_req.path_params[route_info.param_names[i]] = match_results[i + 1].str();
                        }
                    }

                    // 调用 handler 并检查其返回值
                    int response_is_final = route_info.handler(matched_req, res);
                    if (response_is_final == END_HANDING)
                    {
                        return END_HANDING; // 立即返回 END_RESPONSE，表示响应已完成
                    }
                    else if (response_is_final == PROCESSED_INTERNALLY)
                    {
                        return PROCESSED_INTERNALLY; // 立即返回 PROCESSED_INTERNALLY，表示已内部处理
                    }

                    // 如果是 CONTINUE_HANDLING，则继续处理
                    return CONTINUE_HANDLING; // 继续处理
                }
            }
        }
        return std::nullopt; // 未找到匹配
    };

    // 1. 尝试在固定方法中查找错误处理
    auto fixed_it = fixed_method_handlers.find(method);
    if (fixed_it != fixed_method_handlers.end())
    {
        for (const auto &route_info : fixed_it->second)
        {
            auto r = find_and_handle_path(fixed_it->second);
            if (r.has_value())
            {
                if (r.value() == END_HANDING || r.value() == CONTINUE_HANDLING)
                {
                    return;
                }
                else if (r.value() == PROCESSED_INTERNALLY)
                {
                    return;
                }
            }
            // if (route_info.status_code == status_code)
            // {
            //     route_info.handler(req, res);
            //     res.status_code = status_code;
            //     return;
            // }
        }
    }

    // 2. 尝试在正则表达式方法中查找错误处理
    // todo有问题
    for (const auto &regex_pair : regex_method_handlers)
    {
        if (std::regex_match(method, regex_pair.first))
        {
            auto r = find_and_handle_path(regex_pair.second);
            if (r.has_value())
            {
                if (r.value() == END_HANDING || r.value() == CONTINUE_HANDLING)
                {
                    return;
                }
                else if (r.value() == PROCESSED_INTERNALLY)
                {
                    return;
                }
            }
        }
    }
    // for (const auto &regex_pair : regex_method_handlers)
    // {
    //     if (std::regex_match(method, regex_pair.first))
    //     {
    //         for (const auto &route_info : regex_pair.second)
    //         {
    //             auto r = find_and_handle_path(regex_pair.second);
    //             if (r.has_value())
    //             {
    //                 if (r.value() == END_HANDING || r.value() == CONTINUE_HANDLING)
    //                 {
    //                     return;
    //                 }
    //                 else if (r.value() == PROCESSED_INTERNALLY)
    //                 {
    //                     return;
    //                 }
    //             }
    //             // if (route_info.status_code == status_code)
    //             // {
    //             //     route_info.handler(req, res);
    //             //     res.status_code = status_code;
    //             //     return;
    //             // }
    //         }
    //     }
    // }

    // 回退到默认处理
    if (status_code < 100 || status_code > 599)
    {
        status_code = 500; // 非法状态码，回退到 500
    }
    else if (status_code / 100 == 3)
    {
        // 3xx 重定向类状态码，默认不处理
        return;
    }
    res.status_code = status_code;
    res.response_content = std::to_string(status_code) + " " + getHttpCodeText_Pimpl(status_code).value_or("Unknown");
    res.response_headers["status_code"] = std::to_string(res.status_code);
    res.response_headers["Content-Type"] = "text/plain";
}

std::optional<responseContext> cppNetworkUtilPimpl::handleRequest_Pimpl(const requestContext &req)
{
    responseContext res;
    // 设置默认的头
    res.response_headers["Content-Type"] = "*/*";
    res.status_code = 200;
    res.response_headers["connection"] = "close";
    if (req.is_https_connection)
        res.response_headers["Strict-Transport-Security"] = "max-age=31536000; includeSubDomains; preload";

    const std::string &method = req.parsed_request_headers.at("method");
    const std::string &url = req.parsed_request_headers.at("url");

    auto find_and_handle_path = [&](const std::vector<routeInfo> &routes_to_check) -> std::optional<int> {
        for (const auto &route_info : routes_to_check)
        {
            // 允许以 '/' 开头 (固定/模板路径) 或以 '^' 开头 (纯正则表达式路径)
            const std::string &pattern = route_info.path_pattern;
            const int &status_code = route_info.status_code;

            // 检查路径模式是否以 '/' 或 '^' 开头
            if (!pattern.empty() && (pattern[0] == '/' || pattern[0] == '^') &&
                (status_code < 400 || status_code > 600))
            {
                std::smatch match_results;
                // std::cout << "Trying to match URL: " << url << " with pattern: " << pattern << "\n";
                if (std::regex_match(url, match_results, route_info.path_regex))
                {
                    requestContext matched_req = req;
                    for (size_t i = 0; i < route_info.param_names.size(); ++i)
                    {
                        if (i + 1 < match_results.size())
                        {
                            matched_req.path_params[route_info.param_names[i]] = match_results[i + 1].str();
                        }
                    }

                    // 调用 handler 并检查其返回值
                    int response_is_final = route_info.handler(matched_req, res);
                    if (response_is_final == END_HANDING)
                    {
                        return END_HANDING; // 立即返回 END_RESPONSE，表示响应已完成
                    }
                    else if (response_is_final == PROCESSED_INTERNALLY)
                    {
                        return PROCESSED_INTERNALLY; // 立即返回 PROCESSED_INTERNALLY，表示已内部处理
                    }

                    // 如果是 CONTINUE_HANDLING, 则继续处理
                    if (res.status_code >= 300 && res.status_code < 600)
                    {
                        invokeErrorHandler_Pimpl(res.status_code, matched_req, res);
                    }
                    return CONTINUE_HANDLING; // 继续处理
                }
            }
        }
        return std::nullopt; // 未找到匹配
    };

    // 1. 优先匹配固定方法
    auto fixed_it = fixed_method_handlers.find(method);
    if (fixed_it != fixed_method_handlers.end())
    {
        auto r = find_and_handle_path(fixed_it->second);
        if (r.has_value())
        {
            if (r.value() == END_HANDING || r.value() == CONTINUE_HANDLING)
            {
                return res;
            }
            else if (r.value() == PROCESSED_INTERNALLY)
            {
                return std::nullopt;
            }
        }
    }

    // 2. 其次匹配正则表达式方法
    for (const auto &regex_pair : regex_method_handlers)
    {
        if (std::regex_match(method, regex_pair.first))
        {
            auto r = find_and_handle_path(regex_pair.second);
            if (r.has_value())
            {
                if (r.value() == END_HANDING || r.value() == CONTINUE_HANDLING)
                {
                    return res;
                }
                else if (r.value() == PROCESSED_INTERNALLY)
                {
                    return std::nullopt;
                }
            }
        }
    }

    // 都没有匹配，调用 404
    invokeErrorHandler_Pimpl(404, req, res);
    return res;
}