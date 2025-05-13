#include <netinet/in.h>
#include <server/WebSocket.hpp>
#include <server/sha1.hpp>
#include <sys/socket.h>

WebSocket::WebSocket(const int socket_fd) : socket_fd_{socket_fd}, state_{State::CONNECTING} {}

WebSocket::WebSocket(WebSocket &&other) noexcept
    : socket_fd_{std::exchange(other.socket_fd_, -1)}, state_{other.state_},
      message_handler_{std::move(other.message_handler_)} {
  other.state_ = State::CLOSED;
  other.message_handler_ = nullptr;
}

WebSocket &WebSocket::operator=(WebSocket &&other) noexcept {
  if (this != &other) {
    if (socket_fd_ != -1) {
      close(socket_fd_);
    }
    socket_fd_ = std::exchange(other.socket_fd_, -1);
    state_ = other.state_;
    message_handler_ = std::move(other.message_handler_);
    other.state_ = State::CLOSED;
    other.message_handler_ = nullptr;
  }
  return *this;
}

WebSocket::~WebSocket() { close(socket_fd_); }

int WebSocket::get_fd() const { return socket_fd_; }

void WebSocket::send(const std::string &message, OpCode opcode) {}

std::string WebSocket::accept_handshake(const std::string &websocket_key) {
  Chocobo1::SHA1 sha;
  sha.addData((websocket_key.data()), websocket_key.size());
  sha.addData((WS_MAGIC.data()), WS_MAGIC.size());
  sha.finalize();
  const auto digest = sha.toVector();
  return Chocobo1::base64_encode(std::string_view{reinterpret_cast<const char *>(digest.data()), digest.size()});
}

void WebSocket::read_frame(std::string &message, OpCode &opcode) const {
  std::vector<uint8_t> raw_message;
  uint8_t op_code;
  get_websocket_frame(raw_message, op_code);
  // TODO: make std::variant<string_view, std::vector<uint8_t>> for messages like images
  message = std::string(reinterpret_cast<const char *>(raw_message.data()), raw_message.size());
  opcode = to_opcode(op_code);
}

void WebSocket::send_frame() {}

void WebSocket::on_message(MessageHandler handler) { message_handler_ = std::move(handler); }

void WebSocket::get_websocket_frame(std::vector<uint8_t> &raw_message, uint8_t &op_code) const {
  bool is_final_frame = false;
  bool is_first_frame = true;

  do {
    bool fin = false;
    uint8_t opcode = 0;
    std::vector<uint8_t> payload = parse_frame(fin, opcode);

    if (is_first_frame) {
      if (opcode != 0x1 && opcode != 0x2) {
        throw std::runtime_error("Unexpected opcode for initial frame");
      }
      op_code = opcode;
      is_first_frame = false;
    } else {
      if (opcode != 0x0) {
        throw std::runtime_error("Expected continuation frame");
      }
    }
    raw_message.insert(raw_message.end(), payload.begin(), payload.end());
    is_final_frame = fin;
  } while (!is_final_frame);
}

std::vector<uint8_t> WebSocket::parse_frame(bool &out_final, uint8_t &out_opcode) const {
  uint8_t header[2];
  size_t n = recv(socket_fd_, header, sizeof(header), MSG_WAITALL);
  if (n != sizeof(header)) {
    throw std::runtime_error("Failed to read WebSocket frame header");
  }

  const bool fin = (header[0] & 0x80) != 0;
  const uint8_t opcode = header[0] & 0x0F;
  const bool mask = (header[1] & 0x80) != 0;
  uint64_t payloadLen = header[1] & 0x7F;

  if (payloadLen == 126) {
    uint16_t len16;
    n = recv(socket_fd_, &len16, sizeof(len16), MSG_WAITALL);
    if (n != sizeof(len16)) {
      throw std::runtime_error("Failed to read WebSocket frame length");
    }
    payloadLen = ntohs(len16);
  } else if (payloadLen == 127) {
    uint64_t len64;
    n = recv(socket_fd_, &len64, sizeof(len64), MSG_WAITALL);
    if (n != sizeof(len64)) {
      throw std::runtime_error("Failed to read WebSocket frame length");
    }
    payloadLen = be64toh(len64);
  }

  uint8_t maskKey[4] = {};
  if (mask) {
    n = recv(socket_fd_, maskKey, sizeof(maskKey), MSG_WAITALL);
    if (n != sizeof(maskKey)) {
      throw std::runtime_error("Failed to read WebSocket frame mask key");
    }
  }

  // Read payload data
  std::vector<uint8_t> payload(payloadLen);
  if (payloadLen > 0) {
    n = recv(socket_fd_, payload.data(), payloadLen, MSG_WAITALL);
    if (n != static_cast<ssize_t>(payloadLen)) {
      throw std::runtime_error("Failed to read WebSocket frame payload");
    }
  }

  if (mask) {
    for (uint64_t i = 0; i < payloadLen; ++i) {
      payload[i] ^= maskKey[i % 4];
    }
  }

  if (opcode == 0x8) {
    throw std::runtime_error("Connection close frame received");
  }
  if (opcode == 0x9) {
    // TODO: Handle ping frame
    throw std::runtime_error("Make a pong");
  }
  if (opcode == 0xA) {
    // Pong: ignore
  }

  out_final = fin;
  out_opcode = opcode;
  return payload;
}

WebSocket::OpCode WebSocket::to_opcode(const uint8_t raw) const {
  switch (raw) {
    case 0x0: return OpCode::CONTINUATION;
    case 0x1: return OpCode::TEXT;
    case 0x2: return OpCode::BINARY;
    case 0x8: return OpCode::CLOSE;
    case 0x9: return OpCode::PING;
    case 0xA: return OpCode::PONG;
    default: return OpCode::UNKNOWN;
  }
}