#ifndef WSAPP_HPP
#define WSAPP_HPP
#include <functional>
#include <server/SocketListener.hpp>
#include <server/WebSocket.hpp>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>

class WSApplication {
public:
  struct Parameters {
    SocketListener::Parameters socket_listener;
  };
  using OpenHandler = std::function<void(WebSocket &)>;
  using MessageHandler = std::function<void(WebSocket &, std::string_view, WebSocket::OpCode)>;
  using CloseHandler = std::function<void(WebSocket &)>;

  explicit WSApplication(Parameters parameters);

  void activate();

  void stop();

  void add_connection(WebSocket &connection);

  void on_open(OpenHandler handler);

  void on_message(MessageHandler handler);

  void on_close(CloseHandler handler);

private:
  void process_message(int fd);

  Parameters parameters_;
  std::unordered_map<uint64_t, WebSocket> connections;
  uint64_t connection_counter = 0;
  SocketListener socket_listener_;

  std::jthread listener_thread_;

  OpenHandler open_handler_ = nullptr;
  MessageHandler message_handler_ = nullptr;
  CloseHandler close_handler_ = nullptr;
};

#endif //WSAPP_HPP
