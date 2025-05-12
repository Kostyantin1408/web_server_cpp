#include <server/WebSocket.hpp>
#include <server/sha1.hpp>
#include <sys/socket.h>

WebSocket::WebSocket(const int socket_fd)
    : socket_fd_{socket_fd}, state_{State::CONNECTING}, parser_(new websocket_parser{}),
      settings_{new websocket_parser_settings{}} {
  websocket_parser_init(parser_.get());
  websocket_parser_settings_init(settings_.get());

  // hook up our static callbacks
  settings_->on_frame_header = &WebSocket::on_frame_header_cb;
  settings_->on_frame_body = &WebSocket::on_frame_body_cb;
  settings_->on_frame_end = &WebSocket::on_frame_end_cb;

  // allow callbacks to find this instance
  parser_->data = this;
}

WebSocket::~WebSocket() { close(socket_fd_); }

std::string WebSocket::accept_handshake(const std::string &websocket_key) {
  Chocobo1::SHA1 sha;
  sha.addData((websocket_key.data()), websocket_key.size());
  sha.addData((WS_MAGIC.data()), WS_MAGIC.size());
  sha.finalize();
  const auto digest = sha.toVector();
  return Chocobo1::base64_encode(std::string_view{reinterpret_cast<const char *>(digest.data()), digest.size()});
}

void WebSocket::read_frame(std::string &out_payload, OpCode &out_opcode) {
  while (true) {
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      if (!frame_queue_.empty()) {
        auto f = std::move(frame_queue_.front());
        frame_queue_.pop();
        out_opcode = f.opcode;
        out_payload = std::string(f.body.begin(), f.body.end());
        return;
      }
    }

    char buf[4096];
    ssize_t n = ::recv(socket_fd_, buf, sizeof(buf), 0);
    if (n <= 0) {
      state_ = State::CLOSED;
      throw std::runtime_error("socket closed or error");
    }
    // feed bytes into parser
    websocket_parser_execute(parser_.get(), settings_.get(), buf, n);
  }
}

void WebSocket::send_frame(const std::string &payload, OpCode opcode) {
  // build the frame
  size_t frame_size = websocket_calc_frame_size(
      static_cast<websocket_flags>(static_cast<uint8_t>(opcode) | WS_FINAL_FRAME), payload.size());
  std::vector<char> frame(frame_size);
  size_t built =
      websocket_build_frame(frame.data(), static_cast<websocket_flags>(static_cast<uint8_t>(opcode) | WS_FINAL_FRAME),
                            nullptr, // no mask from server
                            payload.data(), payload.size());
  // send all bytes
  size_t offset = 0;
  while (offset < built) {
    ssize_t n = ::send(socket_fd_, frame.data() + offset, built - offset, 0);
    if (n <= 0)
      throw std::runtime_error("send failed");
    offset += n;
  }
}

void WebSocket::on_message(MessageHandler handler) { message_handler_ = std::move(handler); }

int WebSocket::on_frame_header_cb(websocket_parser* p) {
  auto self = static_cast<WebSocket*>(p->data);
  // дізнаємося opcode поточного фрейму
  auto opcode = static_cast<websocket_flags>(websocket_parser_get_opcode(p));

  // очищаємо лише якщо це не продовження фрагментованого повідомлення
  if (opcode != WS_OP_CONTINUE) {
    self->payload_accumulator_.clear();
    // зберігаємо opcode для подальшого використання (якщо треба)
    self->current_opcode_ = opcode;
  }

  return 0;
}

int WebSocket::on_frame_body_cb(websocket_parser* p, const char* at, size_t length) {
  auto self = static_cast<WebSocket*>(p->data);
  // decode masked payload
  std::vector<uint8_t> tmp(length);
  websocket_parser_decode(
    reinterpret_cast<char*>(tmp.data()),
    at,
    length,
    p
  );
  // append to accumulator
  self->payload_accumulator_.insert(
    self->payload_accumulator_.end(),
    tmp.begin(),
    tmp.end()
  );
  return 0;
}

int WebSocket::on_frame_end_cb(websocket_parser* p) {
  auto self = static_cast<WebSocket*>(p->data);
  Frame f;
  // беремо збережений opcode (а не get_opcode, який поверне CONTINUE для всіх наступних фрагментів)
  f.opcode = static_cast<OpCode>(self->current_opcode_);
  f.body   = std::move(self->payload_accumulator_);
  {
    std::lock_guard<std::mutex> lock(self->queue_mutex_);
    self->frame_queue_.push(std::move(f));
  }
  if (self->message_handler_) {
    std::string s(f.body.begin(), f.body.end());
    self->message_handler_(s, f.opcode);
  }
  return 0;
}