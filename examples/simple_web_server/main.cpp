#include <server/WebServer.hpp>
#include <filesystem>
#include <iostream>

int main() {
    WebServer::Parameters params{"127.0.0.1", 8080};
    WebServer server{params};
    const std::filesystem::path base_assets_path = std::filesystem::absolute("assets");
    server.get("/", [base_assets_path](const HttpRequest &req) {
        return HttpResponse::ServeStatic(base_assets_path, req, "/");
    });
    server.post("/test", [](const HttpRequest &req) {
        std::cout << "[POST /test] Received body:\n" << req.body << std::endl;

        return HttpResponse::Text("Received:\n" + req.body, 200);
    });
    server.on_open([](WebSocket &ws) { std::cout << "[WS] Connection opened" << std::endl; })
            .on_message([](WebSocket &ws, std::string_view msg, WebSocket::OpCode opCode) {
                std::cout << "[WS] Message received: " << msg << std::endl;
                ws.send("Thanks for your message!", WebSocket::OpCode::TEXT);
            })
            .on_close([](WebSocket &ws) {
                const int fd = ws.get_fd();
                std::cout << "Connection closed on fd: " << fd << std::endl;
            });
    server.activate_websockets();
    server.run();

    std::cout << "HTTP server is running on http://" << params.host << ":" << params.port << " ..." << std::endl;

    server.wait_for_exit();

    return 0;
}
