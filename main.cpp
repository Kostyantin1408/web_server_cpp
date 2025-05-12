#include "WebServer.hpp"
#include <filesystem>
#include "encryption.hpp"



int main() {
    WebServer server{{"127.0.0.1", 8080}};

    server.get("/hello", [](const HttpRequest& req) {
       return HttpResponse::Text("хуй", 200);
   });
    server.run();
    server.wait_for_exit();
}
