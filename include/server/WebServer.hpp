#ifndef WEBSOCKET___WEBSERVER_HPP
#define WEBSOCKET___WEBSERVER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "server/Threadpool.hpp"
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
    size_t num_threads{4};
    WSApplication::Parameters ws_app{};
  };

  explicit WebServer(Parameters parameters_);

  void get(const std::string &route, RouteHandler handler);
  void post(const std::string &route, RouteHandler handler);
  void put(const std::string &route, RouteHandler handler);
  void del(const std::string &route, RouteHandler handler);
  void patch(const std::string &route, RouteHandler handler);
  void head(const std::string &route, RouteHandler handler);
  void options(const std::string &route, RouteHandler handler);

  WebServer& on_open(WSApplication::OpenHandler handler);
  WebServer& on_message(WSApplication::MessageHandler handler);
  WebServer& on_close(WSApplication::CloseHandler handler);

  void activate_websockets();

  void run();

  void request_stop();

  void wait_for_exit();

  void on_http(int client_fd);

private:
  void listen(int port);

  void main_thread_acceptor(const std::stop_token& token);

  void handle(HttpRequest::HttpMethod method, const std::string &route, RouteHandler handler);

  Parameters parameters;
  int server_fd{};
  sockaddr_in address{};
  std::jthread server_thread;
  std::stop_source stop_source;
  Threadpool thread_pool_;

  bool is_running = false;

  std::unordered_map<HttpRequest::HttpMethod, std::unordered_map<std::string, RouteHandler>> method_handlers;

  std::mutex mtx;
  std::condition_variable cv;

  WSApplication ws_app_;

  bool shutdown_flag = false;
};

#endif // WEBSOCKET___WEBSERVER_HPP
