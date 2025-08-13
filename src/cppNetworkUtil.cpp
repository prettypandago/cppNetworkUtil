#include "cppNetworkUtilPimpl.h" // 这里包含实现类的定义
#include "cppNetworkUtil.h"

#include "defines.h"
#include "log.h"

requestContext cppNetworkUtil::getClientConnectionsInfo(SOCKET client_socket)
{
    return requestContext{pimpl_->client_connections[client_socket].is_https_connection, pimpl_->client_connections[client_socket].ip, pimpl_->client_connections[client_socket].port, pimpl_->client_connections[client_socket].family, pimpl_->client_connections[client_socket].request_data, pimpl_->client_connections[client_socket].request_header, pimpl_->client_connections[client_socket].request_content};
}

std::unordered_map<std::string, std::string> cppNetworkUtil::getParsedHeader(const std::string &header)
{
    return pimpl_->getParsedHeader_Pimpl(header);
}

std::string cppNetworkUtil::getHeaderValue(const std::string &headers, const std::string &key)
{
    return pimpl_->getHeaderValue_Pimpl(headers, key);
}

int cppNetworkUtil::getContentSize(const std::string buffer)
{
    return pimpl_->getContentSize_Pimpl(buffer);
}

int cppNetworkUtil::getPostContentSize(const std::string &buffer)
{
    return pimpl_->getPostContentSize_Pimpl(buffer);
}

std::string cppNetworkUtil::getPostContentType(const std::string &buffer)
{
    return pimpl_->getPostContentType_Pimpl(buffer);
}

std::string cppNetworkUtil::getPostContentBoundary(const std::string &buffer)
{
    return pimpl_->getPostContentBoundary_Pimpl(buffer);
}

std::string cppNetworkUtil::getPostContentBody(const std::string buffer)
{
    return pimpl_->getPostContentBody_Pimpl(buffer);
}

std::string cppNetworkUtil::getHttpCodeText(int code)
{
    return pimpl_->getHttpCodeText_Pimpl(code);
}

std::string cppNetworkUtil::makeResponseHeader(int status, std::unordered_map<std::string, std::string> parameters)
{
    return pimpl_->makeResponseHeader_Pimpl(status, parameters);
}

std::string cppNetworkUtil::makeRequestHeader(std::map<std::string, std::string> parameters)
{
    return pimpl_->makeRequestHeader_Pimpl(parameters);
}

std::string cppNetworkUtil::urlDecode(const std::string &encodedString)
{
    return pimpl_->urlDecode_Pimpl(encodedString);
}

std::map<std::string, std::string> cppNetworkUtil::parseUrlEncodedFormBody(const std::string &encoded_string)
{
    return pimpl_->parseUrlEncodedFormBody_Pimpl(encoded_string);
}

std::vector<std::string> cppNetworkUtil::cutUrlPath(std::string path)
{
    return pimpl_->cutUrlPath_Pimpl(path);
}

std::unordered_map<std::string, std::string> cppNetworkUtil::parseUrlQueryParameters(const std::string &url)
{
    return pimpl_->parseUrlQueryParameters_Pimpl(url);
}

std::map<std::string, multipartData> cppNetworkUtil::parseMultipart(const std::string &boundary, const std::string &body)
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

void cppNetworkUtil::run(int http_port, int https_port)
{
    pimpl_->run_Pimpl(http_port, https_port);
}

void cppNetworkUtil::print_opensslVersion()
{
    pimpl_->print_opensslVersion_Pimpl();
}

void cppNetworkUtil::print_cppNetworkUtilVersion()
{
    pimpl_->print_cppNetworkUtilVersion_Pimpl();
}

void cppNetworkUtil::get(const std::string &path_pattern, routeHandler handler)
{
    pimpl_->registerRoute_Pimpl("GET", path_pattern, std::move(handler));
}

void cppNetworkUtil::post(const std::string &path_pattern, routeHandler handler)
{
    pimpl_->registerRoute_Pimpl("POST", path_pattern, std::move(handler));
}

void cppNetworkUtil::put(const std::string &path_pattern, routeHandler handler)
{
    pimpl_->registerRoute_Pimpl("PUT", path_pattern, std::move(handler));
}

void cppNetworkUtil::delete_(const std::string &path_pattern, routeHandler handler)
{
    pimpl_->registerRoute_Pimpl("DELETE", path_pattern, std::move(handler));
}

void cppNetworkUtil::patch(const std::string &path_pattern, routeHandler handler)
{
    pimpl_->registerRoute_Pimpl("PATCH", path_pattern, std::move(handler));
}

responseContext cppNetworkUtil::handleRequest(const requestContext &req)
{
    return pimpl_->handleRequest_Pimpl(req);
}

void cppNetworkUtil::on(int status_code, routeHandler handler)
{
    return pimpl_->setErrorHandler_Pimpl(status_code, handler);
}