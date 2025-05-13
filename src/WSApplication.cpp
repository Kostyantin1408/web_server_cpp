#include <algorithm>
#include <server/WSApplication.hpp>

WSApplication::WSApplication(Parameters parameters)
    : parameters_{std::move(parameters)},
      socket_listener_{parameters_.socket_listener, [this](const int fd) { process_message(fd); }} {}

void WSApplication::activate() {
  listener_thread_ = std::jthread{[this](const std::stop_token &token) { socket_listener_.run(); }};
}

void WSApplication::stop() {
  socket_listener_.stop();
  listener_thread_.request_stop();
}

void WSApplication::add_connection(WebSocket &connection) {
  const uint64_t connection_id = connection_counter++;
  // TODO: normalize counter id to avoid conflicts and overflows
  auto [it, inserted] = connections.emplace(connection_id, std::move(connection));
  if (!inserted) {
    throw std::runtime_error{"Connection with the same ID already exists"};
  }
  WebSocket& ws = it->second;
  const int connection_fd = ws.get_fd();
  socket_listener_.add_socket(connection_fd);
  if (open_handler_) {
    open_handler_(ws);
  }
}

WSApplication &WSApplication::on_open(OpenHandler handler) {
  open_handler_ = std::move(handler);
  return *this;
}

WSApplication &WSApplication::on_message(MessageHandler handler) {
  message_handler_ = std::move(handler);
  return *this;
}

WSApplication &WSApplication::on_close(CloseHandler handler) {
  close_handler_ = std::move(handler);
  return *this;
}

void WSApplication::process_message(int fd) {
  auto it = std::ranges::find_if(connections, [fd](const auto &pair) { return pair.second.get_fd() == fd; });
  if (it != connections.end()) {
    std::string message;
    WebSocket::OpCode op_code;
    it->second.read_frame(message, op_code);
    message_handler_(it->second, message, op_code);
  }
}
