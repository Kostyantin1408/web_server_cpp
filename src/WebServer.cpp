#include "WebServer.hpp"


static void handle_signal(int signo) {
    if (WebServer::static_server_fd != -1) {
        close(WebServer::static_server_fd);
    }
    _exit(0);
}

WebServer::WebServer() : server_fd(-1), stop_server(false) {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction(SIGTERM)");
        exit(EXIT_FAILURE);
    }
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

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        close(server_fd);
        throw std::runtime_error("bind failed");
    }
    if (::listen(server_fd, 3) < 0) {
        close(server_fd);
        throw std::runtime_error("listen failed");
    }

    std::cout << "HTTP server listening on port " << port << "..." << std::endl;
}

void WebServer::start_accept() {
    while (!stop_server) {
        socklen_t addr_len = sizeof(address);
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&address), &addr_len);
        if (client_fd < 0) {
            if (stop_server)
                break;
            std::cerr << "accept failed" << std::endl;
            continue;
        }
        on_http(client_fd);
        close(client_fd);
    }
}

void WebServer::run() {
    start_accept();
}

void WebServer::on_http(int client_fd) {
    if (m_http_handler) {
        m_http_handler(client_fd);
        return;
    }
    char buffer[1024];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
    }

    const char* http_response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "Hello, World!";

    write(client_fd, http_response, std::strlen(http_response));
}

int WebServer::static_server_fd = -1;

