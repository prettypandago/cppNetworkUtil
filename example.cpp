#include <iostream>

#include "cppNetworkUtil.h"
#include "defines.h"
#include "log.h"

int main(int argc, char **argv)
{
    cppNetworkUtil server;

    // test url: https://127.0.0.1/
    server.get("/", [](const requestContext &req, responseContext &res) { res.response_content = "hello world!"; });

    // test url: https://127.0.0.1/test/123?q=profile
    server.get("/api/users/{id}", [](const requestContext &req, responseContext &res) {
        res.response_content = "User ID: " + req.path_params.at("id") + ", Query: " + req.query_params.at("q");
        res.response_headers["Content-Type"] = "text/plain";
    });

    // test url: https://127.0.0.1/files/document.pdf
    server.get("/files/{name:[a-zA-Z0-9]+}.{ext:[a-zA-Z]+}", [](const requestContext &req, responseContext &res) {
        res.response_content = "File Name: " + req.path_params.at("name") + ", Extension: " + req.path_params.at("ext");
        res.response_headers["Content-Type"] = "text/plain";
    });

    // test url: https://127.0.0.1/notfound
    server.on(404, [](const requestContext &req, responseContext &res) {
        res.response_content = "<html><body><h1>Page Not Found</h1><p>The page you requested does not "
                               "exist.</p></body></html>";
        res.response_headers["Content-Type"] = "text/html";
    });

    try
    {
        server.run(DEFAULT_HTTP_SERVER_PORT, DEFAULT_HTTPS_SERVER_PORT);
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