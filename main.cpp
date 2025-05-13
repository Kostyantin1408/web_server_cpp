#include "server/WebServer.hpp"
#include "server/encryption.hpp"
#include <filesystem>

int main() {
  WebServer server{{"127.0.0.1", 8089}};
  const std::filesystem::path base_assets_path = std::filesystem::absolute("assets");
  server.get("/", [base_assets_path](const HttpRequest &) {
    return HttpResponse::FromFile((base_assets_path / "index.html").string(), "text/html");
  });
  server.get("/assets", [base_assets_path](const HttpRequest &req) {
    return HttpResponse::ServeStatic(base_assets_path, req, "/assets");
  });
  server.post("/test", [](const HttpRequest &req) {
    std::cout << "[POST /test] Received body:\n" << req.body << std::endl;

    return HttpResponse::Text("Received:\n" + req.body, 200);
  });
  server.WSApp().on_open([](WebSocket &ws){std::cout << "[WS] Connection opened" << std::endl;});
  server.WSApp().on_message([](WebSocket &ws, std::string_view msg, WebSocket::OpCode opCode){std::cout << "[WS] Message received: " << msg << std::endl;});
  server.WSApp().activate();
  server.run();
  server.wait_for_exit();
}

// websocket_server.c
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/sha.h>
#include <unistd.h>

#include "server/WebSocketParser.hpp"

#include <algorithm>

// constexpr int PORT = 9002;
// constexpr int BACKLOG = 10;
// constexpr size_t BUF_SZ = 4096;
//
// // --- base64 encoding ---
// static const char b64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
//                                 "abcdefghijklmnopqrstuvwxyz"
//                                 "0123456789+/";
//
// std::string base64_encode(const unsigned char* in, size_t in_len) {
//     std::string out;
//     out.reserve(((in_len + 2) / 3) * 4);
//
//     size_t i = 0;
//     while (i < in_len) {
//         uint32_t octet_a = i < in_len ? static_cast<unsigned char>(in[i++]) : 0;
//         uint32_t octet_b = i < in_len ? static_cast<unsigned char>(in[i++]) : 0;
//         uint32_t octet_c = i < in_len ? static_cast<unsigned char>(in[i++]) : 0;
//         uint32_t triple   = (octet_a << 16) | (octet_b << 8) | octet_c;
//
//         out.push_back(b64_chars[(triple >> 18) & 0x3F]);
//         out.push_back(b64_chars[(triple >> 12) & 0x3F]);
//         out.push_back(b64_chars[(triple >>  6) & 0x3F]);
//         out.push_back(b64_chars[ triple        & 0x3F]);
//     }
//
//     int mod = in_len % 3;
//     if (mod) {
//         out[out.size() - 1] = '=';
//         if (mod == 1) out[out.size() - 2] = '=';
//     }
//     return out;
// }
//
// std::string compute_accept_key(const std::string& client_key) {
//     unsigned char hash[SHA_DIGEST_LENGTH];
//     std::string concat = client_key + WEBSOCKET_UUID;
//     SHA1(reinterpret_cast<const unsigned char*>(concat.data()), concat.size(), hash);
//     return base64_encode(hash, SHA_DIGEST_LENGTH);
// }
//
// void send_handshake(int fd, const std::string& sec_key) {
//     std::string accept = compute_accept_key(sec_key);
//     std::ostringstream resp;
//     resp << "HTTP/1.1 101 Switching Protocols\r\n"
//          << "Upgrade: websocket\r\n"
//          << "Connection: Upgrade\r\n"
//          << "Sec-WebSocket-Accept: " << accept << "\r\n"
//          << "\r\n";
//
//     const auto& resp_str = resp.str();
//     std::cout << "===== HANDSHAKE RESPONSE =====\n" << resp_str
//               << "=============================" << std::endl;
//     if (send(fd, resp_str.data(), resp_str.size(), 0) < 0) {
//         perror("send_handshake");
//     }
// }
//
// struct client_ctx {
//   int fd;
//   websocket_parser parser;
//   websocket_parser_settings settings;
//   websocket_flags last_opcode;
// };
//
// // on_frame_header:
// int on_frame_header(websocket_parser* p) {
//   auto* ctx = static_cast<client_ctx*>(p->data);
//   ctx->last_opcode = static_cast<websocket_flags>(p->flags & WS_OP_MASK);
//   return 0;
// }
//
// // on_frame_body:
// int on_frame_body(websocket_parser* p, const char* at, size_t len) {
//   auto* ctx = static_cast<client_ctx*>(p->data);
//   websocket_flags reply_op = WS_OP_CONTINUE; // дефолт
//
//   switch (ctx->last_opcode) {
//   case WS_OP_TEXT:
//   case WS_OP_BINARY:
//       reply_op = static_cast<websocket_flags>(ctx->last_opcode | WS_FINAL_FRAME);
//     break;
//   case WS_OP_PING:
//     reply_op = static_cast<websocket_flags>(WS_OP_PONG | WS_FINAL_FRAME);
//     break;
//   case WS_OP_CLOSE:
//     reply_op = static_cast<websocket_flags>(WS_OP_CLOSE | WS_FINAL_FRAME);
//     break;
//   default:
//     return 0; // ігноруємо CONTINUATION тощо
//   }
//
//   size_t frame_sz = websocket_calc_frame_size(reply_op, len);
//   std::vector<char> frame(frame_sz);
//   websocket_build_frame(frame.data(), reply_op, nullptr, at, len);
//   if (send(ctx->fd, frame.data(), frame_sz, 0) < 0) perror("send_frame");
//   return 0;
// }
//
// int on_frame_end(websocket_parser*) { return 0; }
//
// void client_thread(int client_fd) {
//     client_ctx ctx{};
//     ctx.fd = client_fd;
//
//     // Read HTTP headers
//     std::string req;
//     std::vector<char> buf(BUF_SZ);
//     ssize_t n;
//     while ((n = recv(client_fd, buf.data(), BUF_SZ, 0)) > 0) {
//         req.append(buf.data(), n);
//         if (req.find("\r\n\r\n") != std::string::npos)
//             break;
//     }
//     if (n <= 0) {
//         close(client_fd);
//         return;
//     }
//
//     std::cout << "----- HANDSHAKE REQUEST -----\n" << req
//               << "-----------------------------" << std::endl;
//
//     // Extract Sec-WebSocket-Key
//     size_t pos = req.find("Sec-WebSocket-Key:");
//     if (pos == std::string::npos) {
//         std::cerr << "No Sec-WebSocket-Key header\n";
//         close(client_fd);
//         return;
//     }
//     pos += strlen("Sec-WebSocket-Key:");
//     while (pos < req.size() && isspace((unsigned char)req[pos])) ++pos;
//     size_t end = req.find("\r\n", pos);
//     std::string key = req.substr(pos, end - pos);
//
//     send_handshake(client_fd, key);
//
//     websocket_parser_init(&ctx.parser);
//     websocket_parser_settings_init(&ctx.settings);
//     ctx.settings.on_frame_header = on_frame_header;
//     ctx.settings.on_frame_body   = on_frame_body;
//     ctx.settings.on_frame_end    = on_frame_end;
//     ctx.parser.data = &ctx;
//
//     // Read frames
//     while ((n = recv(client_fd, buf.data(), BUF_SZ, 0)) > 0) {
//         size_t parsed = websocket_parser_execute(&ctx.parser, &ctx.settings, buf.data(), (size_t)n);
//         if (parsed != (size_t)n) {
//             std::cerr << "Parser error\n";
//             break;
//         }
//     }
//     close(client_fd);
// }
//
// int main() {
//     // Create IPv6 socket for dual-stack
//     int sock = socket(AF_INET6, SOCK_STREAM, 0);
//     if (sock < 0) {
//         perror("socket");
//         return 1;
//     }
//
//     int opt = 1;
//     setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
//     int ipv6only = 0;
//     setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &ipv6only, sizeof(ipv6only));
//
//     sockaddr_in6 addr6{};
//     addr6.sin6_family = AF_INET6;
//     addr6.sin6_port   = htons(PORT);
//     addr6.sin6_addr   = in6addr_any;
//
//     if (bind(sock, reinterpret_cast<sockaddr*>(&addr6), sizeof(addr6)) < 0) {
//         perror("bind");
//         return 1;
//     }
//     if (listen(sock, BACKLOG) < 0) {
//         perror("listen");
//         return 1;
//     }
//
//     std::cout << "WebSocket echo server listening on port " << PORT << " (IPv4 & IPv6)" << std::endl;
//
//     while (true) {
//         sockaddr_storage cli{};
//         socklen_t len = sizeof(cli);
//         int client_fd = accept(sock, reinterpret_cast<sockaddr*>(&cli), &len);
//         if (client_fd < 0) {
//             perror("accept");
//             continue;
//         }
//         // Log client address
//         char host[NI_MAXHOST], serv[NI_MAXSERV];
//         if (getnameinfo(reinterpret_cast<sockaddr*>(&cli), len,
//                         host, sizeof(host), serv, sizeof(serv),
//                         NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
//             std::cout << "[INFO] New connection from " << host << ":" << serv << std::endl;
//         }
//
//         std::thread{client_thread, client_fd}.detach();
//     }
//
//     close(sock);
//     return 0;
// }

#include "server/SocketListener.hpp"
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
