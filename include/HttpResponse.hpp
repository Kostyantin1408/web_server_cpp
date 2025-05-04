#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>
#include <filesystem>
#include <HttpRequest.hpp>

struct HttpResponse {
    std::string version = "HTTP/1.1";
    int status_code = 200;
    std::string status_text = "OK";
    std::map<std::string, std::string> headers;
    std::string body;

    [[nodiscard]] std::string to_string() const;

    void set_header(const std::string& key, const std::string& value);

    static HttpResponse Text(const std::string& body, int status = 200);
    static HttpResponse Json(const std::string& json_body, int status = 200);
    static HttpResponse NotFound(const std::string& message = "404 Not Found");
    static HttpResponse Redirect(const std::string& location, bool permanent = false);
    static HttpResponse Html(const std::string& html, int status = 200);
    static HttpResponse FromFile(const std::string& file_path, const std::string& content_type = "application/octet-stream");
    static HttpResponse ServeStatic(const std::filesystem::path& base_path, const HttpRequest& req, const std::string& route_prefix = "/assets");
    static HttpResponse WebSocketSwitchingProtocols(const std::string& websocket_key);


};

#endif // HTTPRESPONSE_HPP
