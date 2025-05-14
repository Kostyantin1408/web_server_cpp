#include <server/WebServer.hpp>
#include <nlohmann/json.hpp>
#include <uuid/uuid.h>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <filesystem>

using json = nlohmann::json;

struct ClientInfo {
    std::string id;
    std::string name = "Anonymous";
    WebSocket *ws = nullptr;
};

std::unordered_map<int, ClientInfo> clients;
std::vector<json> drawHistory;

std::string generate_uuid() {
    uuid_t uuid;
    uuid_generate_random(uuid);
    char str[37];
    uuid_unparse(uuid, str);
    return {str};
}

void broadcast(const json &msg, int exclude_fd = -1) {
    const std::string raw = msg.dump();

    std::vector<WebSocket *> recipients;
    {
        for (const auto &[fd, client]: clients) {
            if (fd != exclude_fd && client.ws) {
                recipients.push_back(client.ws);
            }
        }
    }

    for (WebSocket *ws : recipients) {
        try {
            ws->send(raw, WebSocket::OpCode::TEXT);
        } catch (const std::exception &e) {
            std::cerr << "[Broadcast Error] " << e.what() << "\n";
        }
    }
}

int main() {
    const int port = 9000;
    const std::filesystem::path base_assets_path = std::filesystem::absolute("public");

    WebServer server{{"127.0.0.1", port}};
    server.get("/", [base_assets_path](const HttpRequest &req) {
        return HttpResponse::ServeStatic(base_assets_path, req, "/");
    });

    server.on_open([](WebSocket &ws) {
        int fd = ws.get_fd();
        std::string uuid = generate_uuid();
        clients[fd] = ClientInfo{uuid, "Anonymous", &ws};
    });

    server.on_message([](WebSocket &ws, std::string_view data, WebSocket::OpCode opCode) {
        int fd = ws.get_fd();
        json msg;
        try {
            msg = json::parse(data);
        } catch (...) {
            return;
        }

        auto it = clients.find(fd);
        if (it == clients.end()) return;
        auto &client = it->second;

        if (msg["type"] == "join") {
            client.name = msg.value("name", "Anonymous");

            for (const auto &stroke : drawHistory) {
                json replay = stroke;
                replay["type"] = "draw";
                ws.send(replay.dump(), WebSocket::OpCode::TEXT);
            }
        } else if (msg["type"] == "draw") {
            json stroke = {
                {"type", "draw"},
                {"fromX", msg["fromX"]},
                {"fromY", msg["fromY"]},
                {"toX", msg["toX"]},
                {"toY", msg["toY"]},
                {"color", msg["color"]},
                {"size", msg["size"]}
            };
            drawHistory.push_back(stroke);
            broadcast(stroke, fd);
        } else if (msg["type"] == "cursor") {
            json cursor = {
                {"type", "cursor"},
                {"x", msg["x"]},
                {"y", msg["y"]},
                {"userId", client.id},
                {"name", client.name}
            };
            broadcast(cursor, fd);
        } else if (msg["type"] == "clear") {
            drawHistory.clear();
            broadcast({{"type", "clear"}});
        }
    });

    server.on_close([](WebSocket &ws) {
        int fd = ws.get_fd();
        clients.erase(fd);
    });

    server.activate_websockets();
    server.run();
    std::cout << "Olivec C++ server running at http://localhost:" << port << std::endl;
    server.wait_for_exit();
}
