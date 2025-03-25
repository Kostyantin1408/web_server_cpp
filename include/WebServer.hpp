#ifndef WEBSOCKET___WEBSERVER_HPP
#define WEBSOCKET___WEBSERVER_HPP

#include <utility>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>
#include <functional>

typedef websocketpp::server<websocketpp::config::asio> server;


class WebServer {
public:
     WebServer() {
        m_server.set_access_channels(websocketpp::log::alevel::app);
        m_server.clear_access_channels(websocketpp::log::alevel::frame_payload);
        m_server.init_asio();
        m_server.set_http_handler(std::bind(&WebServer::on_http, this, std::placeholders::_1));
    }

    void listen(int port) {
        m_server.listen(port);
    }

    void start_accept() {
        m_server.start_accept();
    }

    void run() {
        m_server.run();
    }

private:
    server m_server;
    std::unordered_map<std::string, std::function<void(server::connection_ptr, const std::string&)>> m_handlers;

    void on_http(websocketpp::connection_hdl hdl);
};


#endif //WEBSOCKET___WEBSERVER_HPP
