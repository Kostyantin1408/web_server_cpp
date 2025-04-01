#include "WebServer.hpp"


int main() {
    WebServer server;
    server.listen(1234);
    server.get("/", [](int client_fd, const HttpRequest &req) {
        for (const auto &[key, val] : req.headers) {
            std::cout << key << ": " << val << std::endl;
        }
        const char* response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 13\r\n"
                "\r\n"
                "hello!";
        write(client_fd, response, std::strlen(response));
    });

    server.run();
    return 0;
}

