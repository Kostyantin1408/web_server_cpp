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
  WebSocket &ws = it->second;
  const int connection_fd = ws.get_fd();
  socket_listener_.add_socket(connection_fd, EPOLLIN);
  if (open_handler_) {
    open_handler_(ws);
  }
}

void WSApplication::on_open(OpenHandler handler) {
  open_handler_ = std::move(handler);
}

void WSApplication::on_message(MessageHandler handler) {
  message_handler_ = std::move(handler);
}

void WSApplication::on_close(CloseHandler handler) {
  close_handler_ = std::move(handler);
}

void WSApplication::process_message(int fd) {
  auto it = std::ranges::find_if(connections, [fd](const auto &pair) { return pair.second.get_fd() == fd; });
  if (it == connections.end()) {
    return;
  }

  WebSocket &ws = it->second;
  std::string message;
  WebSocket::OpCode op_code;

  try {
    ws.read_frame(message, op_code);
  } catch (const std::exception &ex) {
    if (close_handler_) {
      close_handler_(ws);
    }
    socket_listener_.remove_socket(fd);
    connections.erase(it);
    return;
  }

  if (op_code == WebSocket::OpCode::CLOSE) {
    if (close_handler_) {
      close_handler_(ws);
    }
    socket_listener_.remove_socket(fd);
    connections.erase(it);
    return;
  }

  if (message_handler_) {
    message_handler_(ws, message, op_code);
  }
}
