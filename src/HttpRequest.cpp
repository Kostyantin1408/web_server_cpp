#include "server/HttpRequest.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

static std::string trim(const std::string &str) {
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start)) ++start;

    auto end = str.end();
    do {
        --end;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}

static std::map<std::string, std::string> parse_query_params(const std::string &query_str) {
    std::map<std::string, std::string> params;
    std::istringstream query_stream(query_str);
    std::string pair;

    while (std::getline(query_stream, pair, '&')) {
        size_t equal_pos = pair.find('=');
        if (equal_pos != std::string::npos) {
            std::string key = pair.substr(0, equal_pos);
            std::string value = pair.substr(equal_pos + 1);
            params[key] = value;
        } else {
            params[pair] = "";
        }
    }

    return params;
}

HttpRequest::HttpMethod HttpRequest::parse_http_method(const std::string &method_str) {
    if (method_str == "GET") {
        return HttpMethod::HTTP_GET;
    }
    if (method_str == "POST") {
        return HttpMethod::HTTP_POST;
    }
    if (method_str == "PUT") {
        return HttpMethod::HTTP_PUT;
    }
    if (method_str == "DELETE") {
        return HttpMethod::HTTP_DELETE;
    }
    if (method_str == "PATCH") {
        return HttpMethod::HTTP_PATCH;
    }
    if (method_str == "HEAD") {
        return HttpMethod::HTTP_HEAD;
    }
    if (method_str == "OPTIONS") {
        return HttpMethod::HTTP_OPTIONS;
    }
    return HttpMethod::HTTP_UNKNOWN;
}

std::string HttpRequest::http_method_to_string(HttpMethod method) {
    if (method == HttpMethod::HTTP_GET) {
        return "GET";
    }
    if (method == HttpMethod::HTTP_POST) {
        return "POST";
    }
    if (method == HttpMethod::HTTP_PUT) {
        return "PUT";
    }
    if (method == HttpMethod::HTTP_DELETE) {
        return "DELETE";
    }
    if (method == HttpMethod::HTTP_PATCH) {
        return "PATCH";
    }
    if (method == HttpMethod::HTTP_HEAD) {
        return "HEAD";
    }
    if (method == HttpMethod::HTTP_OPTIONS) {
        return "OPTIONS";
    }
    return "UNKNOWN";
}

bool HttpRequest::is_websocket_upgrade() const {
    auto it_upgrade = headers.find("upgrade");
    auto it_key = headers.find("sec-websocket-key");

    return it_upgrade != headers.end() &&
           it_key != headers.end() &&
           it_upgrade->second == "websocket";
}

std::string HttpRequest::get_websocket_key() const {
    return headers.at("sec-websocket-key");
}


HttpRequest HttpRequest::parse_http_request(const std::string &raw_request) {
    HttpRequest req;
    std::istringstream stream(raw_request);
    std::string line;

    if (!std::getline(stream, line) || line.empty()) {
        return req;
    }

    std::istringstream req_line(line);
    std::string method_str;
    req_line >> method_str;
    req.method = parse_http_method(method_str);

    std::string full_path;
    req_line >> full_path;
    size_t qmark = full_path.find('?');

    if (qmark != std::string::npos) {
        req.path = full_path.substr(0, qmark);
        std::string query_string = full_path.substr(qmark + 1);
        req.query_params = parse_query_params(query_string);
    } else {
        req.path = full_path;
    }

    req_line >> req.version;

    while (std::getline(stream, line) && line != "\r") {
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = trim(line.substr(0, colon));
            std::string value = trim(line.substr(colon + 1));
            if (!value.empty() && value.back() == '\r') {
                value.pop_back();
            }
            std::string key_lower = key;
            std::ranges::transform(key_lower, key_lower.begin(), ::tolower);
            req.headers[key_lower] = value;
        }
    }

    std::ostringstream body_stream;
    while (std::getline(stream, line)) {
        body_stream << line << "\n";
    }
    req.body = body_stream.str();
    if (!req.body.empty() && req.body.back() == '\n') {
        req.body.pop_back();
    }

    return req;
}
