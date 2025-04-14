#include "WebServer.hpp"

#include <cstring>
#include <iostream>
#include <unistd.h>

int main() {
  WebServer server{{"127.0.0.1", 1234}};
  server.get("/", [](int client_fd, const HttpRequest &req) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "Content-Length: 13\r\n"
                           "\r\n"
                           "hello!";
    write(client_fd, response, std::strlen(response));
  });

  server.get("/hello", [](int client_fd, const HttpRequest &req) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "Content-Length: 13\r\n"
                           "\r\n"
                           "hello_there!";
    write(client_fd, response, std::strlen(response));
  });

  server.run();

  server.wait_for_exit();
  return 0;
}
