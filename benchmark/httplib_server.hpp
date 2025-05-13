#ifndef HTTPLIB_SERVER_HPP
#define HTTPLIB_SERVER_HPP

#include "httplib.h"
#include <thread>

inline std::unique_ptr<httplib::Server> launch_httplib_server(int port = 8080) {
    auto server = std::make_unique<httplib::Server>();
    server->Get("/hello", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("OK", "text/plain");
    });

    std::thread([server_ptr = server.get(), port]() {
        server_ptr->listen("0.0.0.0", port);
    }).detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return server;
}

#endif //HTTPLIB_SERVER_HPP
