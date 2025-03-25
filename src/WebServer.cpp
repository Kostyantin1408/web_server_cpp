#include "WebServer.hpp"

#include <utility>

void WebServer::on_http(websocketpp::connection_hdl hdl) {
    server::connection_ptr con = m_server.get_con_from_hdl(std::move(hdl));

    std::string resource = con->get_resource();

    std::ostringstream oss;
    oss << con->get_request().get_method() << " " << resource << " HTTP/1.1\r\n";
    oss << "Content-Type: " << con->get_request_header("Content-Type") << "\r\n";
    oss << "Content-Length: " << con->get_request_header("Content-Length") << "\r\n";
    oss << "Accept: " << con->get_request_header("Accept") << "\r\n";
    oss << "User-Agent: " << con->get_request_header("User-Agent") << "\r\n";
    oss << "\r\n";
    oss << con->get_request_body();
    std::string full_message = oss.str();
}


