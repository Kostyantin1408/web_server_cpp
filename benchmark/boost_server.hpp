#ifndef BOOST_SERVER_HPP
#define BOOST_SERVER_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <thread>
#include <memory>
#include <atomic>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class BoostWebServer {
public:
    BoostWebServer(int port) : ioc_(1), acceptor_(ioc_, {tcp::v4(), static_cast<unsigned short>(port)}), stop_(false) {}

    void run() {
        while (!stop_) {
            tcp::socket socket{ioc_};
            beast::error_code ec;
            acceptor_.accept(socket, ec);
            if (ec) continue;

            boost_thread = std::jthread(&BoostWebServer::handle_session, this, std::move(socket));
        }
    }

    void stop() {
        stop_ = true;
        beast::error_code ec;
        acceptor_.close(ec);
        boost_thread.request_stop();
    }

private:
    void handle_session(tcp::socket socket) {
        beast::tcp_stream stream(std::move(socket));
        beast::flat_buffer buffer;

        http::request<http::string_body> req;
        beast::error_code ec;
        http::read(stream, buffer, req, ec);
        if (ec) return;

        if (req.target() == "/hello") {
            http::response<http::string_body> res{
                http::status::ok, req.version()};
            res.set(http::field::server, "Boost.Beast");
            res.set(http::field::content_type, "text/plain");
            res.body() = "OK";
            res.prepare_payload();

            http::write(stream, res, ec);
            stream.socket().shutdown(tcp::socket::shutdown_send, ec);
        }
    }
    std::jthread boost_thread;
    net::io_context ioc_;
    tcp::acceptor acceptor_;
    std::atomic<bool> stop_;
};

inline std::unique_ptr<BoostWebServer> launch_boost_server(int port = 8082) {
    auto server = std::make_unique<BoostWebServer>(port);
    std::thread([&server]() {
        server->run();
    }).detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return server;
}

#endif // BOOST_SERVER_HPP
