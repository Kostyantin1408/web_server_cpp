#ifndef WEBSOCKET_HPP
#define WEBSOCKET_HPP
#include <cstdint>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

class WebSocket {
public:
  enum class State { CONNECTING, OPEN, CLOSING, CLOSED };
  enum class OpCode : uint8_t { CONTINUATION = 0x0, TEXT = 0x1, BINARY = 0x2, CLOSE = 0x8, PING = 0x9, PONG = 0xA, UNKNOWN};

  struct WebSocketFrame {
    bool fin;
    uint8_t opcode;
    bool masked;
    uint64_t payload_length;
    uint8_t masking_key[4];
    std::vector<uint8_t> payload;
  };

  using MessageHandler = std::function<void(const std::string &, OpCode)>;

  explicit WebSocket(int socket_fd);

  WebSocket(WebSocket&& other) noexcept;

  WebSocket& operator=(WebSocket &&) noexcept;

  ~WebSocket();

  int get_fd() const;

  void send(const std::string &message, OpCode opcode) const;

  void read_frame(std::string &message, OpCode &opcode) const;

  std::string accept_handshake(const std::string &websocket_key);

  void on_message(MessageHandler handler);

private:
  void get_websocket_frame(std::vector<uint8_t> &raw_message, uint8_t& op_code) const;

  void send_frame(const std::vector<uint8_t> &data, OpCode opcode, bool is_final = true) const;

  std::vector<uint8_t> parse_frame(bool& out_final, uint8_t& out_opcode) const;

  OpCode to_opcode(uint8_t raw) const;

  std::vector<uint8_t> payload_accumulator_;
  std::queue<WebSocketFrame> frame_queue_;
  std::mutex queue_mutex_;

  int socket_fd_;
  State state_;
  MessageHandler message_handler_;

  static constexpr ssize_t MAX_TEXT_FRAME_SIZE = 4096;
  static constexpr ssize_t MAX_BINARY_FRAME_SIZE = 16384;
  static constexpr std::string_view WS_MAGIC = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
};

#endif // WEBSOCKET_HPP
