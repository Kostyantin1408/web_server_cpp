#include "WebServer.hpp"
#include <filesystem>
#include "encryption.hpp"



int main() {

//    std::string client_key = "dGhlIHNhbXBsZSBub25jZQ==";
//    std::string accept   = compute_accept_key(client_key);
//    std::cout << "Sec-WebSocket-Accept: " << accept << "\n";
//

    WebServer server{{"127.0.0.1", 8080}};

    const std::filesystem::path base_assets_path = std::filesystem::absolute("assets");

    server.get("/", [base_assets_path](const HttpRequest &) {
        return HttpResponse::FromFile((base_assets_path / "index.html").string(), "text/html");
    });

    server.get("/assets", [base_assets_path](const HttpRequest& req) {
        return HttpResponse::ServeStatic(base_assets_path, req, "/assets");
    });

    server.post("/test", [](const HttpRequest& req) {
       std::cout << "[POST /test] Received body:\n" << req.body << std::endl;

       return HttpResponse::Text("Received:\n" + req.body, 200);
   });

    server.run();
    std::this_thread::sleep_for(std::chrono::minutes(10));
}
