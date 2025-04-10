#include "WebServer.hpp"

#include <cstring>
#include <iostream>
#include <unistd.h>

int main() {
  WebServer server{{"127.0.0.1", 8080}};
  server.get("/", [](int client_fd, const HttpRequest &req) {
    for (const auto &[key, val] : req.headers) {
      std::cout << key << ": " << val << std::endl;
    }
    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "Content-Length: 6\r\n"
                           "\r\n"
                           "hello!";
    write(client_fd, response, std::strlen(response));
  });

  server.run();
  server.wait_for_exit();
  return 0;
}
