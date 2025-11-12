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
    networkutil.on("GET", "/", [&networkutil](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/plain";
        res.response_content = "Hello, world!";
        return END_HANDING;
    });

    // Test url: https://127.0.0.1/test/123?q=profile
    // Response: "User ID: 123, Query: profile"
    // url parameters: {id: 123}
    // query parameters: {q: profile}
    networkutil.on("GET", "/test/{id}", [&networkutil](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/plain";
        res.response_content = "User ID: " + req.path_params.at("id") + ", Query: " + req.query_params.at("q");
        return END_HANDING;
    });

    // Test url: https://127.0.0.1/files/document.pdf
    // Response: "File Name: document, Extension: pdf"
    // Url path support regex
    networkutil.on("GET", "/files/{name:[a-zA-Z0-9]+}.{ext:[a-zA-Z]+}",
                   [&networkutil](const requestContext &req, responseContext &res) {
                       res.response_headers["Content-Type"] = "text/plain";
                       res.response_content =
                           "File Name: " + req.path_params.at("name") + ", Extension: " + req.path_params.at("ext");
                       return END_HANDING;
                   });

    // Test url: https://127.0.0.1/catch-all/a
    networkutil.on("GET", "^/catch-all/.*", [&networkutil](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/plain";
        res.response_content = "This route catches all GET requests to /catch-all/ and its subpaths.";
        return END_HANDING;
    });

    // Test url: https://127.0.0.1/badrequest
    // Response: Bad Request
    // This route is intentionally set to return a 400 Bad Request status code.
    // This is useful for Testing error handling.
    networkutil.on("GET", "/badrequest", [&networkutil](const requestContext &req, responseContext &res) {
        res.status_code = 400; // Bad Request
        return CONTINUE_HANDLING;
    });

    // Test url: https://127.0.0.1/forallmethod
    // Response: Your method is: (request method, e.g., GET, POST, etc.)
    // Set method to "*" to handle all methods
    networkutil.on("*", "/forallmethod", [&networkutil](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/plain";
        res.response_content = "Your method is: " + req.parsed_request_headers.at("method");
        return END_HANDING;
    });

    // Test url: https://127.0.0.1/getandpost
    // Handles all GET and POST requests
    // Method support regex
    networkutil.on("^GET|POST$", "/getandpost", [&networkutil](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/plain";
        res.response_content = "This handles both GET and POST!";
        return END_HANDING;
    });

    // Test url: https://127.0.0.1/circuitbreaker
    // Circuit breaker Responses
    // Response This response is sent immediately without further processing.
    networkutil.on("GET", "/circuitbreaker", [&networkutil](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/plain";
        res.response_content = "This response is sent immediately without further processing.";
        return END_HANDING; // Return END_HANDING to send the response immediately
        res.response_content += "You will not see this part.";
    });

    // Test url: https://127.0.0.1/onlyfirst
    // This response body can only be accessed once.
    networkutil.on("GET", "/onlyfirst", [&networkutil](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/plain";
        res.response_content = "This response is delete.";
        networkutil.off("GET", "/onlyfirst"); // Unregister this route after first use (the path must be register path)
        return END_HANDING;                   // Return END_HANDING to send the response immediately
    });

    // Test url: https://127.0.0.1/name?name=yourname
    // Response: Name: yourname
    // Return to CONTINUE_HANDLING to continue processing (e.g., for error handling)
    // If the "name" query parameter is missing, it sets a 400 status code
    networkutil.on("GET", "/name", [&networkutil](const requestContext &req, responseContext &res) {
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

        return END_HANDING;
    });

    // Test url: https://127.0.0.1/wait?t=3
    // Response: This response is sent after a delay.
    networkutil.on("GET", "/wait", [&networkutil](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] =
            "text/html"; // Must set to text/html for chunked transfer encoding, otherwise
                         // browser will not display the content until the entire response is received.

        networkutil.beginDataChunkStreamTransfer(res);

        // Simulate a long processing time (e.g., 3 seconds)
        std::string time_str;

        auto time_it = req.query_params.find("t");
        if (time_it != req.query_params.end())
        {
            time_str = time_it->second;
        }
        if (time_str.empty())
        {
            res.status_code = 400; // Bad Request
            return CONTINUE_HANDLING;
        }

        int time = std::stoi(time_str);

        networkutil.sendDataToSocket(req.client_socket,
                                     networkutil.makeResponseHeader(res.status_code, res.response_headers));

        networkutil.sendDataChunkToSocket(req.client_socket, "<p>Start processing...</p>");

        for (int i = 0; i < time; i++)
        {
            networkutil.sendDataChunkToSocket(req.client_socket,
                                              "<p>Processing... " + std::to_string(i) + " seconds elapsed.</p>");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        networkutil.sendDataChunkToSocket(req.client_socket, "<p>This response is sent after a " +
                                                                 std::to_string(time) + " seconds delay.</p>");

        networkutil.endDataChunkStreamTransfer(req.client_socket);
        return PROCESSED_INTERNALLY;
    });

    // Test url: https://127.0.0.1/continuerouting
    // The example for continue routing
    networkutil.on("GET", "/continuerouting", [&networkutil](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/plain";
        res.response_content = "This is the first handler. Continuing routing to the next handler.\n";
        return CONTINUE_ROUTING; // Continue routing to the next matching route
    });
    networkutil.on("GET", "/continuerouting", [&networkutil](const requestContext &req, responseContext &res) {
        res.response_content += "This is the second handler.";
        return END_HANDING;
    });

    // Test url: https://127.0.0.1/api/v1/notfound
    // Response: Page Not Found
    // The page you requested does not exist.
    // This route is error page for handle.
    networkutil.on("GET", 404, "^/api/v1/.*", [&networkutil](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "application/json";
        res.response_content = "{\"error\": \"Page Not Found\"}";
        return END_HANDING;
    });

    // Test url: https://127.0.0.1/notfound
    // Response: Page Not Found
    // The page you requested does not exist.
    // This route is error page for handle.
    networkutil.on("GET", 404, [&networkutil](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/html";
        res.response_content = "<html><body><h1>Page Not Found</h1><p>The page you requested does not "
                               "exist.</p></body></html>";
        return END_HANDING;
    });

    // Test url: https://127.0.0.1/badrequest
    // Response: Bad request
    // The request could not be understood by the networkutil due to malformed syntax. Please check your request and
    // try again.
    // Set method to "*" (or ".*", it will automatically convert "*" to ".*") to handle all methods
    networkutil.on("*", 400, [&networkutil](const requestContext &req, responseContext &res) {
        res.response_headers["Content-Type"] = "text/html";
        res.response_content =
            "<html><body><h1>Bad request</h1><p>The request could not be understood by the networkutil "
            "due to malformed syntax. Please check your request and try again."
            "</p></body></html>";
        return END_HANDING;
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