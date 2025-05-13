#ifndef CROW_SERVER_HPP
#define CROW_SERVER_HPP

#include <crow.h>
#include <thread>
#include <memory>
#include <atomic>

class CrowServerWrapper {
public:
    explicit CrowServerWrapper(int port)
        : app_(std::make_unique<crow::SimpleApp>()), port_(port), stop_requested_(false) {
        crow::logger::setLogLevel(crow::LogLevel::CRITICAL);

        CROW_ROUTE((*app_), "/hello")
        ([] {
            return "OK";
        });

        thread_ = std::thread([this] {
            app_->port(port_).multithreaded().run();
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void stop() {
        if (!stop_requested_) {
            stop_requested_ = true;
            app_->stop();
            if (thread_.joinable()) {
                thread_.join();
            }
        }
    }

    ~CrowServerWrapper() {
        stop();
    }

private:
    std::unique_ptr<crow::SimpleApp> app_;
    std::thread thread_;
    int port_;
    std::atomic<bool> stop_requested_;
};

inline std::unique_ptr<CrowServerWrapper> launch_crow_server(int port = 8083) {
    return std::make_unique<CrowServerWrapper>(port);
}

#endif // CROW_SERVER_HPP
