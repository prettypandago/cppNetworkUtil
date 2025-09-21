#include <iostream>

#include "cppNetworkUtil.h"
#include "defines.h"
#include "log.h"

int main(int argc, char **argv)
{
    cppNetworkUtil networkutil;

    // Test url: https://127.0.0.1/
    // Response: "Hello, world!"
    // Simple GET request
    networkutil.on("GET", "/", [](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/plain";
        res.response_content = "Hello, world!";
        return END_RESPONSE;
    });

    // Test url: https://127.0.0.1/test/123?q=profile
    // Response: "User ID: 123, Query: profile"
    // url parameters: {id: 123}
    // query parameters: {q: profile}
    networkutil.on("GET", "/test/{id}", [](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/plain";
        res.response_content = "User ID: " + req.path_params.at("id") + ", Query: " + req.query_params.at("q");
        return END_RESPONSE;
    });

    // Test url: https://127.0.0.1/files/document.pdf
    // Response: "File Name: document, Extension: pdf"
    // Url path support regex
    networkutil.on("GET", "/files/{name:[a-zA-Z0-9]+}.{ext:[a-zA-Z]+}",
                   [](const requestContext &req, responseContext &res) {
                       res.response_headers["Content-Type"] = "text/plain";
                       res.response_content =
                           "File Name: " + req.path_params.at("name") + ", Extension: " + req.path_params.at("ext");
                       return END_RESPONSE;
                   });

    // Test url: https://127.0.0.1/badrequest
    // Response: Bad Request
    // This route is intentionally set to return a 400 Bad Request status code.
    // This is useful for Testing error handling.
    networkutil.on("GET", "/badrequest", [](const requestContext &req, responseContext &res) {
        res.status_code = 400; // Bad Request
        return END_RESPONSE;
    });

    // Test url: https://127.0.0.1/forallmethod
    // Response: Your method is: (request method, e.g., GET, POST, etc.)
    // Set method to "*" to handle all methods
    networkutil.on("*", "/forallmethod", [](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/plain";
        res.response_content = "Your method is: " + req.parsed_request_headers.at("method");
        return END_RESPONSE;
    });

    // Test url: https://127.0.0.1/getandpost
    // Handles all GET and POST requests
    // Method support regex
    networkutil.on("^GET|POST$", "/getandpost", [](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/plain";
        res.response_content = "This handles both GET and POST!";
        return END_RESPONSE;
    });

    // Test url: https://127.0.0.1/circuitbreaker
    // Circuit breaker Responses
    // Response This response is sent immediately without further processing.
    networkutil.on("GET", "/circuitbreaker", [](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/plain";
        res.response_content = "This response is sent immediately without further processing.";
        return END_RESPONSE; // Return END_RESPONSE to send the response immediately
        res.response_content += "You will not see this part.";
    });

    // Test url: https://127.0.0.1/name?name=yourname
    // Response: Name: yourname
    // Return to CONTINUE_HANDLING to continue processing (e.g., for error handling)
    // If the "name" query parameter is missing, it sets a 400 status code
    networkutil.on("GET", "/name", [](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/plain";

        std::string name;

        auto name_it = req.query_params.find("name");
        if (name_it != req.query_params.end())
        {
            name = name_it->second;
        }
        if (name.empty())
        {
            res.status_code = 400; // Bad Request
            return CONTINUE_HANDLING;
        }
        res.response_content = "Name: " + name;

        return END_RESPONSE;
    });

    // Test url: https://127.0.0.1/notfound
    // Response: Page Not Found
    // The page you requested does not exist.
    // This route is error page for handle.
    networkutil.on("GET", 404, [](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/html";
        res.response_content = "<html><body><h1>Page Not Found</h1><p>The page you requested does not "
                               "exist.</p></body></html>";
        return END_RESPONSE;
    });

    // Test url: https://127.0.0.1/badrequest
    // Response: Bad request
    // The request could not be understood by the networkutil due to malformed syntax. Please check your request and try
    // again.
    // Set method to "*" (or ".*", it will automatically convert "*" to ".*") to handle all methods
    networkutil.on("*", 400, [](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/html";
        res.response_content =
            "<html><body><h1>Bad request</h1><p>The request could not be understood by the networkutil "
            "due to malformed syntax. Please check your request and try again."
            "</p></body></html>";
        return END_RESPONSE;
    });

    try
    {
        networkutil.run(DEFAULT_HTTP_SERVER_PORT, DEFAULT_HTTPS_SERVER_PORT,
                        BEHAVIOR_MODE_REDIRECT_HTTP_REQUEST_TO_HTTPS, IP_PROTOCOL_MODE_IPV4_AND_IPV6_BOTH,
                        DEFAULT_CERT_PATH, DEFAULT_KEY_PATH, DEFAULT_PRINT_LISTEN_INFO);
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