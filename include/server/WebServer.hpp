#ifndef WEBSOCKET___WEBSERVER_HPP
#define WEBSOCKET___WEBSERVER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "WSApplication.hpp"

#include <condition_variable>
#include <functional>
#include <netinet/in.h>
#include <queue>
#include <stop_token>
#include <thread>

class WebServer {
public:
  using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

  struct Parameters {
    std::string host{};
    int port{};
    WSApplication::Parameters ws_app{};
  };

  struct WSParams {
    std::function<void(WebSocket *)> on_open = nullptr;
    std::function<void(WebSocket *)> on_message = nullptr;
  };

  explicit WebServer(Parameters parameters_);

  WSApplication& WSApp();

  void simulate_on_open();

  void run();

  void get(const std::string &route, RouteHandler handler);
  void post(const std::string &route, RouteHandler handler);
  void put(const std::string &route, RouteHandler handler);
  void del(const std::string &route, RouteHandler handler);
  void patch(const std::string &route, RouteHandler handler);
  void head(const std::string &route, RouteHandler handler);
  void options(const std::string &route, RouteHandler handler);


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

  std::unordered_map<HttpRequest::HttpMethod, std::unordered_map<std::string, RouteHandler>> method_handlers;
  void handle(HttpRequest::HttpMethod method, const std::string& route, RouteHandler handler);

  static constexpr int number_of_workers = 8;

  std::vector<std::jthread> thread_pool;
  std::queue<int> tasks_queue;
  std::mutex mtx;
  std::condition_variable cv;

  WSApplication ws_app_;

  bool shutdownFlag = false;
};

#endif // WEBSOCKET___WEBSERVER_HPP
