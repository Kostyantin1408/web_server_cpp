#ifndef WEBSOCKET___WEBSERVER_HPP
#define WEBSOCKET___WEBSERVER_HPP

#include "HttpRequest.hpp"
#include <functional>
#include <netinet/in.h>
#include <queue>
#include <condition_variable>
#include <stop_token>
#include <thread>

class WebServer {
public:
  struct Parameters {
    std::string host{};
    int port{};
  };

  explicit WebServer(Parameters parameters_);

  void run();

  void post(const std::string &route, std::function<void(int, HttpRequest)> handler);

  void get(const std::string &route, std::function<void(int, HttpRequest)> handler);

  void stop();

  void wait_for_exit();

  void on_http(int client_fd);

private:
  void listen(int port);
  void main_thread_acceptor(const std::stop_token& token);
  void worker();

  Parameters parameters;
  int server_fd{};
  sockaddr_in address{};
  std::jthread server_thread;
  std::stop_source stop_source;

  bool is_running = false;

  std::unordered_map<std::string, std::function<void(int, HttpRequest)>> get_handlers;
  std::unordered_map<std::string, std::function<void(int, HttpRequest)>> post_handlers;

  static constexpr int number_of_workers = 8;

  std::vector<std::jthread> thread_pool;
  std::queue<int> tasks_queue;
  std::mutex mtx;
  std::condition_variable cv;

  bool shutdownFlag = false;
};

#endif // WEBSOCKET___WEBSERVER_HPP
