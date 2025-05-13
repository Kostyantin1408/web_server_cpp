#include <netinet/in.h>
#include <server/WebSocket.hpp>
#include <sha1.hpp>
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

void WebSocket::send(const std::string &message, const OpCode opcode) const {
  if (opcode != OpCode::TEXT && opcode != OpCode::BINARY) {
    throw std::runtime_error("Invalid opcode for message");
  }
  if (opcode == OpCode::TEXT) {
    send_frame(std::vector<uint8_t>{message.begin(), message.end()}, opcode);
  } else {
    const size_t message_length = message.size();
    const size_t frames_count = (message_length + MAX_BINARY_FRAME_SIZE - 1) / MAX_BINARY_FRAME_SIZE;
    for (size_t i = 0; i < frames_count; ++i) {
      const size_t start = i * MAX_BINARY_FRAME_SIZE;
      const size_t end = std::min(start + MAX_BINARY_FRAME_SIZE, message_length);
      send_frame(std::vector<uint8_t>{message.begin() + start, message.begin() + end}, opcode, i == frames_count - 1);
    }
  }
}

void WebSocket::read_frame(std::string &message, OpCode &opcode) const {
  std::vector<uint8_t> raw_message;
  uint8_t op_code;
  get_websocket_frame(raw_message, op_code);
  // TODO: make std::variant<string_view, std::vector<uint8_t>> for messages like images
  message = std::string(reinterpret_cast<const char *>(raw_message.data()), raw_message.size());
  opcode = to_opcode(op_code);
}

std::string WebSocket::accept_handshake(const std::string &websocket_key) {
  Chocobo1::SHA1 sha;
  sha.addData((websocket_key.data()), websocket_key.size());
  sha.addData((WS_MAGIC.data()), WS_MAGIC.size());
  sha.finalize();
  const auto digest = sha.toVector();
  return Chocobo1::base64_encode(std::string_view{reinterpret_cast<const char *>(digest.data()), digest.size()});
}

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

void WebSocket::send_frame(const std::vector<uint8_t> &data, const OpCode opcode, bool is_final) const {
  std::vector<uint8_t> header;

  uint8_t byte1 = 0;
  if (is_final) {
    byte1 |= 0x80; // Set FIN bit
  }
  byte1 |= static_cast<uint8_t>(opcode) & 0x0F;
  header.push_back(byte1);

  const uint64_t payloadLen = data.size();
  if (payloadLen < 126) {
    header.push_back(static_cast<uint8_t>(payloadLen));
  } else if (payloadLen <= 0xFFFF) {
    header.push_back(126);
    const uint16_t len16 = htons(static_cast<uint16_t>(payloadLen));
    header.push_back(len16 >> 8 & 0xFF);
    header.push_back(len16 & 0xFF);
  } else {
    header.push_back(127);
    const uint64_t len64 = htobe64(payloadLen);
    for (int i = 0; i < 8; ++i) {
      header.push_back(len64 >> 56 - 8 * i & 0xFF);
    }
  }

  size_t sent = 0;
  while (sent < header.size()) {
    const ssize_t n = ::send(socket_fd_, header.data() + sent, header.size() - sent, 0);
    if (n < 0) {
      throw std::runtime_error("Failed to send WebSocket frame header");
    }
    sent += n;
  }

  sent = 0;
  while (sent < data.size()) {
    const ssize_t n = ::send(socket_fd_, data.data() + sent, data.size() - sent, 0);
    if (n < 0) {
      throw std::runtime_error("Failed to send WebSocket frame payload");
    }
    sent += n;
  }
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
  case 0x0:
    return OpCode::CONTINUATION;
  case 0x1:
    return OpCode::TEXT;
  case 0x2:
    return OpCode::BINARY;
  case 0x8:
    return OpCode::CLOSE;
  case 0x9:
    return OpCode::PING;
  case 0xA:
    return OpCode::PONG;
  default:
    return OpCode::UNKNOWN;
  }
}