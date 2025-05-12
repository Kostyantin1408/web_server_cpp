#include <server/WSApplication.hpp>

void WSApplication::activate() {

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
