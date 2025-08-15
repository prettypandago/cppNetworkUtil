#include <iostream>

#include "cppNetworkUtil.h"
#include "defines.h"
#include "log.h"

int main(int argc, char **argv)
{
    cppNetworkUtil server;

    // test url: https://127.0.0.1/
    // response: "Hello, world!"
    // simple GET request
    server.on("GET", "/",
              [](const requestContext &req, responseContext &res) { res.response_content = "Hello, world!"; });

    // test url: https://127.0.0.1/test/123?q=profile
    // response: "User ID: 123, Query: profile"
    // url parameters: {id: 123}
    // query parameters: {q: profile}
    server.on("GET", "/test/{id}", [](const requestContext &req, responseContext &res) {
        res.response_content = "User ID: " + req.path_params.at("id") + ", Query: " + req.query_params.at("q");
        res.response_headers["Content-Type"] = "text/plain";
    });

    // test url: https://127.0.0.1/files/document.pdf
    // response: "File Name: document, Extension: pdf"
    // url path support regex
    server.on("GET", "/files/{name:[a-zA-Z0-9]+}.{ext:[a-zA-Z]+}", [](const requestContext &req, responseContext &res) {
        res.response_content = "File Name: " + req.path_params.at("name") + ", Extension: " + req.path_params.at("ext");
        res.response_headers["Content-Type"] = "text/plain";
    });

    // test url: https://127.0.0.1/badrequest
    // response: Bad Request
    // This route is intentionally set to return a 400 Bad Request status code.
    // This is useful for testing error handling.
    server.on("GET", "/badrequest", [](const requestContext &req, responseContext &res) {
        res.status = 400; // Bad Request
    });

    // test url: https://127.0.0.1/forallmethod
    // response: Your method is: (request method, e.g., GET, POST, etc.)
    // set method to "*" to handle all methods
    server.on("*", "/forallmethod", [](const requestContext &req, responseContext &res) {
        res.response_content = "Your method is: " + req.parsed_request_headers.at("method");
        res.response_headers["Content-Type"] = "text/plain";
    });

    // test url: https://127.0.0.1/notfound
    // response: Page Not Found
    // The page you requested does not exist.
    // This route is error page for handle.
    server.on("GET", 404, [](const requestContext &req, responseContext &res) {
        res.response_content = "<html><body><h1>Page Not Found</h1><p>The page you requested does not "
                               "exist.</p></body></html>";
        res.response_headers["Content-Type"] = "text/html";
    });

    // test url: https://127.0.0.1/badrequest
    // response: Bad request
    // The request could not be understood by the server due to malformed syntax. Please check your request and try
    // again.
    // set method to "*" to handle all methods
    server.on("*", 400, [](const requestContext &req, responseContext &res) {
        res.response_content = "<html><body><h1>Bad request</h1><p>The request could not be understood by the server "
                               "due to malformed syntax. Please check your request and try again."
                               "</p></body></html>";
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