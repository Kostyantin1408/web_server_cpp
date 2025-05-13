#ifndef CROW_SERVER_HPP
#define CROW_SERVER_HPP

#include <crow.h>
#include <thread>
#include <memory>
#include <atomic>
#include <future>

class CrowServerWrapper {
public:
    explicit CrowServerWrapper(int port)
        : port_(port) {
        crow::logger::setLogLevel(crow::LogLevel::CRITICAL);

        CROW_ROUTE(app, "/hello")([] {
            return "OK";
        });
        future = app.port(port_).multithreaded().run_async();
    }

    void stop() {
        app.stop();

        if (future.valid()) {
            future.get();
        }
    }

    ~CrowServerWrapper() {
        stop();
    }

private:
    crow::SimpleApp app;
    int port_;
    std::future<void> future;
};

inline std::unique_ptr<CrowServerWrapper> launch_crow_server(int port = 8083) {
    return std::make_unique<CrowServerWrapper>(port);
}

#endif // CROW_SERVER_HPP
