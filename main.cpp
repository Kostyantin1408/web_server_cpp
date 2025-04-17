#include "WebServer.hpp"
#include <iostream>
#include <filesystem>

int main() {
    WebServer server{{"127.0.0.1", 8080}};

    const std::filesystem::path base_assets_path = std::filesystem::absolute("assets");

    server.get("/", [base_assets_path](int, const HttpRequest &) {
        return HttpResponse::FromFile((base_assets_path / "index.html").string(), "text/html");
    });

    server.get("/assets", [base_assets_path](int, const HttpRequest& req) {
        return HttpResponse::ServeStatic(base_assets_path, req, "/assets");
    });

    server.run();
    std::this_thread::sleep_for(std::chrono::minutes(10));
}
