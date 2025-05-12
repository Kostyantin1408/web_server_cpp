#ifndef WEBSOCKET_HPP
#define WEBSOCKET_HPP
#include <server/WebSocketParser.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <vector>

class WebSocket {
public:
  enum class State { CONNECTING, OPEN, CLOSING, CLOSED };
  enum class OpCode : uint8_t { CONTINUATION = 0x0, TEXT = 0x1, BINARY = 0x2, CLOSE = 0x8, PING = 0x9, PONG = 0xA };

  struct Frame {
    std::vector<uint8_t> body;
    OpCode opcode;
  };

  using MessageHandler = std::function<void(const std::string &, OpCode)>;

  explicit WebSocket(int socket_fd);

  ~WebSocket();

  std::string accept_handshake(const std::string &websocket_key);

  void read_frame(std::string &out_payload, OpCode &out_opcode);

  void send_frame(const std::string &payload, OpCode opcode = OpCode::TEXT);

  void on_message(MessageHandler handler);

private:
  static int on_frame_header_cb(websocket_parser *p);
  static int on_frame_body_cb(websocket_parser *p, const char *at, size_t length);
  static int on_frame_end_cb(websocket_parser *p);

  std::unique_ptr<websocket_parser> parser_;
  std::unique_ptr<websocket_parser_settings> settings_;
  std::vector<uint8_t> payload_accumulator_;
  std::queue<Frame> frame_queue_;
  std::mutex queue_mutex_;
  websocket_flags current_opcode_;

  int socket_fd_;
  State state_;
  MessageHandler message_handler_;

  static constexpr std::string_view WS_MAGIC = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
};

#endif // WEBSOCKET_HPP
