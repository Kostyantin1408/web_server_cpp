#include "WebServer.hpp"
#include <filesystem>
#include "encryption.hpp"


int main() {
    int port = 8080;
    WebServer server{{"127.0.0.1", port}};
    const std::filesystem::path base_assets_path = std::filesystem::absolute("assets");
    server.get("/", [base_assets_path](const HttpRequest &req) {
        return HttpResponse::ServeStatic(base_assets_path, req, "/");
    });
    server.run();
    std::cout << "HTTP server listening on port " << port << "..." << std::endl;

    std::this_thread::sleep_for(std::chrono::minutes(10));
}
