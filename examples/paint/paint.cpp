#include <server/WebServer.hpp>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

struct ClientInfo {
    std::string id;
    std::string name;
};

int main() {
    auto port = 9000;
    WebServer server{{"127.0.0.1", port}};
    const std::filesystem::path base_assets_path = std::filesystem::absolute("public");

    std::unordered_map<int, ClientInfo> clients;
    std::unordered_map<int, WebSocket *> ws_connections;
    std::vector<nlohmann::json> drawHistory;

    auto broadcast = [&](const nlohmann::json &msg, int exclude_fd = -1) {
        const std::string data = msg.dump();
        std::vector<int> disconnected_fds;

        for (auto &[fd, ws]: ws_connections) {
            if (fd != exclude_fd) {
                try {
                    ws->send(data, WebSocket::OpCode::TEXT);
                } catch (const std::exception &e) {
                    std::cerr << "Error sending to fd " << fd << ": " << e.what() << std::endl;
                    disconnected_fds.push_back(fd);
                }
            }
        }

        for (auto fd: disconnected_fds) {
            clients.erase(fd);
            ws_connections.erase(fd);
        }
    };

    server.get("/", [base_assets_path](const HttpRequest &req) {
        return HttpResponse::ServeStatic(base_assets_path, req, "/");
    });

    server.on_open([&](WebSocket &ws) {
        int fd = ws.get_fd();
        clients[fd] = {"", "Anonymous"};
        ws_connections[fd] = &ws;
        std::cout << "[WS] Connected fd: " << fd << std::endl;
    });

    server.on_message([&](WebSocket &ws, std::string_view msg, WebSocket::OpCode opCode) {
        int fd = ws.get_fd();
        auto json_msg = nlohmann::json::parse(msg, nullptr, false);
        if (json_msg.is_discarded()) {
            std::cerr << "Failed to parse JSON\n";
            return;
        }

        const std::string &type = json_msg["type"];
        if (type == "join") {
            clients[fd].name = json_msg.value("name", "Anonymous");
            for (const auto &stroke: drawHistory)
                ws.send(stroke.dump(), WebSocket::OpCode::TEXT);
        } else if (type == "draw") {
            nlohmann::json stroke = {
                {"type", "draw"},
                {"fromX", json_msg["fromX"]},
                {"fromY", json_msg["fromY"]},
                {"toX", json_msg["toX"]},
                {"toY", json_msg["toY"]},
                {"color", json_msg["color"]},
                {"size", json_msg["size"]}
            };
            drawHistory.push_back(stroke);
            broadcast(stroke, fd);
        } else if (type == "cursor") {
            nlohmann::json cursor = {
                {"type", "cursor"},
                {"userId", clients[fd].id},
                {"name", clients[fd].name},
                {"x", json_msg["x"]},
                {"y", json_msg["y"]}
            };
            broadcast(cursor, fd);
        } else if (type == "clear") {
            drawHistory.clear();
            broadcast({{"type", "clear"}});
        }
    });

    server.on_close([&](WebSocket &ws) {
        int fd = ws.get_fd();
        clients.erase(fd);
        ws_connections.erase(fd);
        std::cout << "[WS] Disconnected fd: " << fd << std::endl;
    });

    server.activate_websockets();
    server.run();

    std::cout << "Server running at http://localhost:" << port << "\n";
    server.wait_for_exit();
}
