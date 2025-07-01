#include "cppNetworkUtilPimpl.h" // 这里包含实现类的定义
#include "cppNetworkUtil.h"
#include "serverCallback.h"

#include "defines.h"
#include "log.h"

std::string cppNetworkUtil::get_client_connections_ip(SOCKET client_socket)
{
    return pimpl_->client_connections[client_socket].ip; // 获取客户端 IP 地址
}

int cppNetworkUtil::get_client_connections_port(SOCKET client_socket)
{
    return pimpl_->client_connections[client_socket].port; // 获取客户端端口号
}

std::string cppNetworkUtil::get_client_connections_recv_buffer(SOCKET client_socket)
{
    return pimpl_->client_connections[client_socket].recv_buffer; // 获取客户端接收缓冲区
}

std::string cppNetworkUtil::getHeaderMethod(const std::string buffer)
{
    return pimpl_->getHeaderMethod_Pimpl(buffer);
}

std::string cppNetworkUtil::getGetHeaderUrl(const std::string buffer)
{
    return pimpl_->getGetHeaderUrl_Pimpl(buffer);
}

int cppNetworkUtil::getContentSize(const std::string buffer)
{
    return pimpl_->getContentSize_Pimpl(buffer);
}

std::string cppNetworkUtil::getHeaderValue(const std::string &headers, const std::string &key)
{
    return pimpl_->getHeaderValue_Pimpl(headers, key);
}

std::string cppNetworkUtil::getPostContentBody(const std::string buffer)
{
    return pimpl_->getPostContentBody_Pimpl(buffer);
}

std::string cppNetworkUtil::buildResponseHeader(responseHeaderParameters parameters)
{
    return pimpl_->buildResponseHeader_Pimpl(parameters);
}

std::string cppNetworkUtil::buildRequestHeader(requestHeaderParameters parameter)
{
    return pimpl_->buildRequestHeader_Pimpl(parameter);
}

std::string cppNetworkUtil::urlDecode(const std::string &encodedString)
{
    return pimpl_->urlDecode_Pimpl(encodedString);
}

std::vector<std::string> cppNetworkUtil::getURLParameterRestfulapi(std::string url)
{
    return pimpl_->getURLParameterRestfulapi_Pimpl(url);
}

std::map<std::string, std::string> cppNetworkUtil::parseUrlQueryParameters(const std::string &url)
{
    return pimpl_->parseUrlQueryParameters_Pimpl(url);
}

std::vector<multipartData> cppNetworkUtil::parseMultipart(const std::string &boundary, const std::string &body)
{
    return pimpl_->parseMultipart_Pimpl(boundary, body);
}

void cppNetworkUtil::sendDataToHttpSocket(SOCKET socket, const std::string data)
{
    pimpl_->sendDataToHttpSocket_Pimpl(socket, data);
}

void cppNetworkUtil::sendDataToHttpsSocket(SOCKET socket, const std::string data)
{
    pimpl_->sendDataToHttpsSocket_Pimpl(socket, data);
}

void cppNetworkUtil::sendDataToHttpHost(const std::string &host, const std::string &path, int port, std::string &header, std::string &content)
{
    pimpl_->sendDataToHttpHost_Pimpl(host, path, port, header, content);
}

void cppNetworkUtil::sendDataToHttpsHost(const std::string &host, const std::string &path, int port, std::string &header, std::string &content, bool enable_CA)
{
    pimpl_->sendDataToHttpsHost_Pimpl(host, path, port, header, content, enable_CA);
}

cppNetworkUtil::cppNetworkUtil() : pimpl_(std::make_unique<cppNetworkUtilPimpl>()) {}

cppNetworkUtil::~cppNetworkUtil() = default; // unique_ptr 会自动管理内存

void cppNetworkUtil::printOpensslVersion()
{
    pimpl_->printOpensslVersion_Pimpl();
}

void cppNetworkUtil::run(int port, serverCallback *callback)
{
    pimpl_->run_Pimpl(port, callback);
}