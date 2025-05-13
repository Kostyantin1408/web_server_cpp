#include <WebServer.hpp>
#include <HttpRequest.hpp>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <encryption.hpp>
#include <iostream>
#include <mutex>
#include <utility>
#include <algorithm>
#include <ranges>
#include "HttpRequestReader.hpp"


WebServer::WebServer(Parameters parameters_) : parameters{std::move(parameters_)} {
    listen(parameters.port);
}

void WebServer::listen(int port) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::runtime_error("socket creation failed");
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_fd);
        throw std::runtime_error("setsockopt failed");
    }

    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        close(server_fd);
        throw std::runtime_error("bind failed");
    }
    if (::listen(server_fd, 3) < 0) {
        close(server_fd);
        throw std::runtime_error("listen failed");
    }

}

void WebServer::run() {
    server_thread = std::jthread{[this](const std::stop_token &stop_token) { main_thread_acceptor(stop_token); }};
    thread_pool.reserve(number_of_workers);
    for (size_t i = 0; i < number_of_workers; ++i) {
        thread_pool.emplace_back([this]() { worker(); });
    }
    stop_source = server_thread.get_stop_source();
}

void WebServer::main_thread_acceptor(const std::stop_token &token) {
    while (!token.stop_requested()) {
        socklen_t addr_len = sizeof(address);
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr *>(&address), &addr_len);
        if (client_fd < 0) {
            std::cerr << "accept failed" << std::endl;
            continue;
        } {
            std::unique_lock<std::mutex> lock(mtx);
            tasks_queue.push(client_fd);
        }
        cv.notify_one();
    }
}

void WebServer::worker() {
    while (true) {
        int client_fd; {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&] { return !tasks_queue.empty() || shutdownFlag; });

            if (shutdownFlag && tasks_queue.empty())
                break;

            client_fd = tasks_queue.front();
            tasks_queue.pop();
        }
        on_http(client_fd);
        close(client_fd);
    }
}

void WebServer::handle(HttpRequest::HttpMethod method, const std::string &route, RouteHandler handler) {
    method_handlers[method][route] = std::move(handler);
}

void WebServer::get(const std::string &route, RouteHandler handler) {
    handle(HttpRequest::HttpMethod::HTTP_GET, route, std::move(handler));
}

void WebServer::post(const std::string &route, RouteHandler handler) {
    handle(HttpRequest::HttpMethod::HTTP_POST, route, std::move(handler));
}

void WebServer::put(const std::string &route, RouteHandler handler) {
    handle(HttpRequest::HttpMethod::HTTP_PUT, route, std::move(handler));
}

void WebServer::del(const std::string &route, RouteHandler handler) {
    handle(HttpRequest::HttpMethod::HTTP_DELETE, route, std::move(handler));
}

void WebServer::patch(const std::string &route, RouteHandler handler) {
    handle(HttpRequest::HttpMethod::HTTP_PATCH, route, std::move(handler));
}

void WebServer::head(const std::string &route, RouteHandler handler) {
    handle(HttpRequest::HttpMethod::HTTP_HEAD, route, std::move(handler));
}

void WebServer::options(const std::string &route, RouteHandler handler) {
    handle(HttpRequest::HttpMethod::HTTP_OPTIONS, route, std::move(handler));
}


void WebServer::stop() {
    if (!stop_source.request_stop()) {
        throw std::runtime_error("Failed to request server stop.");
    } {
        std::lock_guard<std::mutex> lock(mtx);
        shutdownFlag = true;
    }
    cv.notify_all();

    shutdown(server_fd, SHUT_RDWR);

    wait_for_exit();

    close(server_fd);
}

void WebServer::wait_for_exit() {
    for (auto &worker_thread: thread_pool) {
        if (worker_thread.joinable())
            worker_thread.join();
    }
}


void WebServer::on_http(int client_fd) {
    bool keep_alive = true;

    while (keep_alive) {
        HttpRequestReader reader(client_fd);
        auto raw_request_opt = reader.read_full_request();
        if (!raw_request_opt) break;

        const std::string &raw_request = *raw_request_opt;
        HttpRequest req = HttpRequest::parse_http_request(raw_request);
        if (req.method == HttpRequest::HttpMethod::HTTP_UNKNOWN) {
            HttpResponse response = HttpResponse::NotFound("Unknown HTTP method");
            std::string resp_str = response.to_string();
            write(client_fd, resp_str.c_str(), resp_str.size());
            break;
        }

        if (req.is_websocket_upgrade()) {
            HttpResponse ws_response = HttpResponse::WebSocketSwitchingProtocols(req.get_websocket_key());
            std::string resp_str = ws_response.to_string();
            write(client_fd, resp_str.c_str(), resp_str.size());
            break;
        }

        HttpResponse response;
        const auto &handlers = method_handlers[req.method];

        auto exact_match = handlers.find(req.path);
        if (exact_match != handlers.end()) {
            response = exact_match->second(req);
        } else {
            bool matched = false;
            for (const auto &[route_prefix, handler]: handlers) {
                if (!route_prefix.empty() && req.path.starts_with(route_prefix)) {
                    response = handler(req);
                    matched = true;
                    break;
                }
            }

            if (!matched) {
                response = HttpResponse::NotFound("404 Not Found: " + req.path);
            }
        }

        auto conn_it = req.headers.find("connection");
        std::string conn_value = (conn_it != req.headers.end()) ? conn_it->second : "";
        std::ranges::transform(conn_value, conn_value.begin(), ::tolower);

        if (conn_value == "close" || req.version != "HTTP/1.1") {
            keep_alive = false;
        } else {
            response.set_header("Connection", "keep-alive");
        }

        std::string resp_str = response.to_string();
        write(client_fd, resp_str.c_str(), resp_str.size());
    }
}
