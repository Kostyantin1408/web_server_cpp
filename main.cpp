#include "WebServer.hpp"

int main() {
    WebServer server{};
    server.listen(9001);
    server.start_accept();
    server.run();
    return 0;
}
