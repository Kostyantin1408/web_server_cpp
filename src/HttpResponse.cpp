#include "HttpResponse.hpp"

#include <encryption.hpp>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <unordered_map>

std::string HttpResponse::to_string() const {
    std::ostringstream response;

    response << version << " " << status_code << " " << status_text << "\r\n";
    for (const auto &[key, value]: headers) {
        response << key << ": " << value << "\r\n";
    }
    if (headers.find("Content-Length") == headers.end()) {
        response << "Content-Length: " << body.size() << "\r\n";
    }
    response << "\r\n";
    response << body;

    return response.str();
}

void HttpResponse::set_header(const std::string &key, const std::string &value) {
    headers[key] = value;
}

HttpResponse HttpResponse::Text(const std::string &body, const int status) {
    HttpResponse res;
    res.status_code = status;
    res.status_text = "OK";
    res.body = body;
    res.set_header("Content-Type", "text/plain");
    return res;
}

HttpResponse HttpResponse::Json(const std::string &json_body, const int status) {
    HttpResponse res;
    res.status_code = status;
    res.status_text = "OK";
    res.body = json_body;
    res.set_header("Content-Type", "application/json");
    return res;
}

HttpResponse HttpResponse::NotFound(const std::string &message) {
    HttpResponse res;
    res.status_code = 404;
    res.status_text = "Not Found";
    res.body = message;
    res.set_header("Content-Type", "text/plain");
    return res;
}

HttpResponse HttpResponse::Redirect(const std::string &location, bool permanent) {
    HttpResponse res;
    res.status_code = permanent ? 301 : 302;
    res.status_text = permanent ? "Moved Permanently" : "Found";
    res.set_header("Location", location);
    res.set_header("Content-Type", "text/plain");
    res.body = "Redirecting to " + location;
    return res;
}

HttpResponse HttpResponse::Html(const std::string &html, int status) {
    HttpResponse res;
    res.status_code = status;
    res.status_text = "OK";
    res.body = html;
    res.set_header("Content-Type", "text/html");
    return res;
}

HttpResponse HttpResponse::FromFile(const std::string &file_path, const std::string &content_type) {
    HttpResponse res;
    std::ifstream file(file_path, std::ios::binary);

    if (!file.is_open()) {
        return NotFound("File not found: " + file_path);
    }

    std::ostringstream content;
    content << file.rdbuf();
    file.close();

    res.body = content.str();
    res.status_code = 200;
    res.status_text = "OK";
    res.set_header("Content-Type", content_type);
    return res;
}

static std::string detect_mime(std::string ext) {
    std::ranges::transform(ext, ext.begin(), ::tolower);

    static const std::unordered_map<std::string, std::string> mime_types = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".svg", "image/svg+xml"},
        {".json", "application/json"},
        {".txt", "text/plain"}
    };

    auto it = mime_types.find(ext);
    return it != mime_types.end() ? it->second : "application/octet-stream";
}

HttpResponse HttpResponse::ServeStatic(const std::filesystem::path &base_path, const HttpRequest &req,
                                       const std::string &route_prefix) {
    if (!req.path.starts_with(route_prefix)) {
        return NotFound("Invalid static path: " + req.path);
    }

    std::string relative = req.path.substr(route_prefix.length());
    if (!relative.empty() && relative[0] == '/')
        relative.erase(0, 1);

    std::filesystem::path requested_path = base_path / relative;

    std::error_code ec;
    std::filesystem::path normalized_base = weakly_canonical(base_path, ec);
    std::filesystem::path normalized_path = weakly_canonical(requested_path, ec);

    if (ec) {
        return NotFound("Canonicalization error.");
    }

    auto base_it = normalized_base.begin();
    auto path_it = normalized_path.begin();
    for (; base_it != normalized_base.end() && path_it != normalized_path.end(); ++base_it, ++path_it) {
        if (*base_it != *path_it) {
            return NotFound("Access denied.");
        }
    }

    if (!exists(normalized_path) || is_directory(normalized_path)) {
        return NotFound("File not found: " + normalized_path.string());
    }

    std::string ext = normalized_path.extension().string();
    std::ranges::transform(ext, ext.begin(), ::tolower);
    std::string mime = detect_mime(ext);

    std::cerr << "Serving static file: " << normalized_path << std::endl;
    return FromFile(normalized_path.string(), mime);
}

HttpResponse HttpResponse::WebSocketSwitchingProtocols(const std::string &websocket_key) {
    HttpResponse res;
    res.status_code = 101;
    res.status_text = "Switching Protocols";
    res.set_header("Upgrade", "websocket");
    res.set_header("Connection", "Upgrade");
    res.set_header("Sec-WebSocket-Accept", compute_accept_key(websocket_key));
    res.body = "";
    return res;
}
