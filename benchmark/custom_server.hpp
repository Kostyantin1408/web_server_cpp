#ifndef CUSTOM_SERVER_HPP
#define CUSTOM_SERVER_HPP

#include <memory>

#include <server/HttpRequest.hpp>
#include <server/WebServer.hpp>


inline std::unique_ptr<WebServer> launch_custom_server(int port = 8080) {
    auto server = std::make_unique<WebServer>(WebServer::Parameters{.host = "127.0.0.1", .port = port});
    server->get("/hello", [](const HttpRequest&) {
        return HttpResponse::Text("OK");
    });
    server->run();
    return server;
}

#endif //CUSTOM_SERVER_HPP
