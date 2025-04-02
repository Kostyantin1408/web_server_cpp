#ifndef WEBSOCKET___WEBSERVER_HPP
#define WEBSOCKET___WEBSERVER_HPP

#include "HttpRequest.hpp"
#include <functional>
#include <netinet/in.h>
#include <stop_token>
#include <thread>

class WebServer {
public:
  struct Parameters {
    std::string host{};
    int port{};
  };

  WebServer(Parameters parameters_);

  void run();

  void post(const std::string &route, std::function<void(int, HttpRequest)> handler);

  void get(const std::string &route, std::function<void(int, HttpRequest)> handler);

  void worker(std::stop_token stop_token);

  void stop();

  void wait_for_exit();

  static int static_server_fd;

private:
  void listen(int port);

  Parameters parameters;
  int server_fd;
  sockaddr_in address;
  std::jthread server_thread;
  std::stop_source stop_source;

  bool is_running = false;

  std::unordered_map<std::string, std::function<void(int, HttpRequest)>> get_handlers;
  std::unordered_map<std::string, std::function<void(int, HttpRequest)>> post_handlers;
  void on_http(int client_fd);
};

#endif // WEBSOCKET___WEBSERVER_HPP
