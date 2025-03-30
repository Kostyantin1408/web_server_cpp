#ifndef WEBSOCKET___WEBSERVER_HPP
#define WEBSOCKET___WEBSERVER_HPP

#include <utility>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>
#include <functional>


class WebServer {
public:
    WebServer();

    void listen(int port);

    void start_accept();

    void run();

    static int static_server_fd;

private:
    int server_fd;
    sockaddr_in address;
    volatile bool stop_server;

    std::function<void(int)> m_http_handler;

    void on_http(int client_fd);

};


#endif //WEBSOCKET___WEBSERVER_HPP
