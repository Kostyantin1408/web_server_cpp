#include <WebServer.hpp>
#include <HttpRequest.hpp>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

WebServer::WebServer(Parameters parameters_) : parameters{std::move(parameters_)} {
  listen(parameters.port);
}

void WebServer::listen(int port) {
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  static_server_fd = server_fd;
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

  std::cout << "HTTP server listening on port " << port << "..." << std::endl;
}

void WebServer::run() {
  server_thread = std::jthread{[this](const std::stop_token &stop_token) { worker(stop_token); }};
  stop_source = server_thread.get_stop_source();
}

void WebServer::get(const std::string &route, std::function<void(int, HttpRequest)> handler) {
  get_handlers[route] = handler;
}

void WebServer::post(const std::string &route, std::function<void(int, HttpRequest)> handler) {
  post_handlers[route] = handler;
}

void WebServer::worker(std::stop_token) {
  while (!stop_source.stop_requested()) {
    socklen_t addr_len = sizeof(address);
    int client_fd = accept(server_fd, reinterpret_cast<sockaddr *>(&address), &addr_len);
    if (client_fd < 0) {
      std::cerr << "accept failed" << std::endl;
      continue;
    }
    on_http(client_fd);
    close(client_fd);
  }
}

void WebServer::request_stop() {
  if (!stop_source.request_stop()) {
    throw std::runtime_error("Failed to request server stop.");
  }

  shutdown(server_fd, SHUT_RDWR);

  wait_for_exit();

  close(server_fd);
  std::cout << "Server stopped." << std::endl;
}

void WebServer::wait_for_exit() {
  server_thread.join();
}


void WebServer::on_http(int client_fd) {
  char buffer[1024];
  ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

  if (bytes_read > 0) {
    buffer[bytes_read] = '\0';

    std::string request_raw(buffer);
    HttpRequest req = parse_http_request(request_raw);
    if (req.method == "GET") {
      get_handlers[req.path](client_fd, req);
    } else if (req.method == "POST") {
      post_handlers[req.path](client_fd, req);
    }
  }
}

int WebServer::static_server_fd = -1;
