#ifndef WSAPP_HPP
#define WSAPP_HPP
#include <server/WebSocket.hpp>
#include <functional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class WSApplication {
public:
  using OpenHandler = std::function<void(WebSocket &)>;
  using MessageHandler = std::function<void(WebSocket &)>;
  using CloseHandler = std::function<void(WebSocket &)>;

  void activate();

  WSApplication& on_open(OpenHandler handler);

  WSApplication& on_message(MessageHandler handler);

  WSApplication& on_close(CloseHandler handler);

private:
  std::unordered_map<int, WebSocket> connections;

  OpenHandler open_handler_ = nullptr;
  MessageHandler message_handler_ = nullptr;
  CloseHandler close_handler_ = nullptr;
};

#endif //WSAPP_HPP
