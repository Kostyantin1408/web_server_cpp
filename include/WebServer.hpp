#ifndef WEBSOCKET___WEBSERVER_HPP
#define WEBSOCKET___WEBSERVER_HPP

#include <utility>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>
#include <functional>
#include "HttpRequest.hpp"


class WebServer {
public:
    WebServer();

    void listen(int port);

    void start_accept();

    void run();

    void post(const std::string &route, std::function<void(int, HttpRequest)> handler);

    void get(const std::string &route, std::function<void(int, HttpRequest)> handler);


    static int static_server_fd;

private:
    int server_fd;
    sockaddr_in address;
    volatile bool stop_server;

    std::unordered_map<std::string, std::function<void(int, HttpRequest)>> get_handlers;
    std::unordered_map<std::string, std::function<void(int, HttpRequest)>> post_handlers;
    void on_http(int client_fd);

};


#endif //WEBSOCKET___WEBSERVER_HPP
