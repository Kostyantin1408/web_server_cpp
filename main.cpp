#include "WebServer.hpp"


int main() {
    WebServer server;
    server.listen(1234);
    server.run();
    return 0;
}

