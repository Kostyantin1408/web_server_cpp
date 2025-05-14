#include <algorithm>
#include <cstring>
#include <iostream>
#include <mutex>
#include <ranges>
#include <server/HttpRequest.hpp>
#include <server/WebServer.hpp>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

WebServer::WebServer(Parameters parameters_)
    : parameters{std::move(parameters_)}, thread_pool_{parameters.num_threads}, ws_app_{std::move(parameters_.ws_app)} {
    listen(parameters.port);
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

WebServer &WebServer::on_open(WSApplication::OpenHandler handler) {
    ws_app_.on_open(std::move(handler));
    return *this;
}

WebServer &WebServer::on_message(WSApplication::MessageHandler handler) {
    ws_app_.on_message(std::move(handler));
    return *this;
}

WebServer &WebServer::on_close(WSApplication::CloseHandler handler) {
    ws_app_.on_close(std::move(handler));
    return *this;
}

void WebServer::activate_websockets() { ws_app_.activate(); }

void WebServer::run() {
    server_thread = std::jthread{[this](const std::stop_token &st) { main_thread_acceptor(st); }};
    stop_source = server_thread.get_stop_source();
}

void WebServer::request_stop() {
    if (!stop_source.request_stop()) {
        throw std::runtime_error("Failed to request server stop.");
    } {
        std::lock_guard lock(mtx);
        shutdown_flag = true;
    }
    cv.notify_all();

    thread_pool_.request_stop();

    ws_app_.stop();

    shutdown(server_fd, SHUT_RDWR);

    wait_for_exit();

    close(server_fd);
}

void WebServer::wait_for_exit() {
    std::unique_lock lock(mtx);
    cv.wait(lock, [this] { return shutdown_flag; });
    thread_pool_.wait_for_exit();
}

void WebServer::on_http(int client_fd) {
    char buffer[1024];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';

        std::string request_raw(buffer);
        HttpRequest req = HttpRequest::parse_http_request(request_raw);
        if (req.method == HttpRequest::HttpMethod::HTTP_UNKNOWN) {
            HttpResponse response = HttpResponse::NotFound("Unknown HTTP method");
            write(client_fd, response.to_string().c_str(), response.to_string().size());
            close(client_fd);
            return;
        }

        if (req.is_websocket_upgrade()) {
            WebSocket ws{client_fd};
            ws_app_.add_connection(ws);
            HttpResponse ws_response =
                    HttpResponse::WebSocketSwitchingProtocols(ws.accept_handshake(req.get_websocket_key()));
            std::string resp_str = ws_response.to_string();
            write(client_fd, resp_str.c_str(), resp_str.size());
            return;
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

        std::string resp_str = response.to_string();
        write(client_fd, resp_str.c_str(), resp_str.size());
        close(client_fd);
    }
}

void WebServer::listen(const int port) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::runtime_error("socket creation failed");
    }

    const int opt = 1;
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

void WebServer::main_thread_acceptor(const std::stop_token &token) {
    while (!token.stop_requested()) {
        socklen_t addr_len = sizeof(address);
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr *>(&address), &addr_len);
        if (client_fd < 0) {
            continue;
        } {
            std::scoped_lock lock(mtx);
            if (shutdown_flag) {
                close(client_fd);
                break;
            }
        }
        thread_pool_.submit([this, client_fd]() { on_http(client_fd); });
    }
}

void WebServer::handle(const HttpRequest::HttpMethod method, const std::string &route, RouteHandler handler) {
    method_handlers[method][route] = std::move(handler);
}
