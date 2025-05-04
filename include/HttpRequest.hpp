#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <optional>

struct HttpRequest {
    enum class HttpMethod {
        HTTP_GET,
        HTTP_POST,
        HTTP_PUT,
        HTTP_DELETE,
        HTTP_PATCH,
        HTTP_HEAD,
        HTTP_OPTIONS,
        HTTP_UNKNOWN
    };


    HttpMethod method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
    std::map<std::string, std::string> query_params;

    std::optional<std::string> upgrade_header;
    std::optional<std::string> websocket_key;
    std::optional<std::string> websocket_version;

    static HttpMethod parse_http_method(const std::string& method_str);
    static std::string http_method_to_string(HttpMethod method);

    static HttpRequest parse_http_request(const std::string &raw_request);
    static bool is_websocket_upgrade(const HttpRequest &req);
};


#endif //HTTPREQUEST_HPP
