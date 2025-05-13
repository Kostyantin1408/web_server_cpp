#include "server/HttpRequestReader.hpp"
#include <sys/socket.h>
#include <algorithm>
#include <sstream>

HttpRequestReader::HttpRequestReader(int socket_fd)
    : fd(socket_fd) {}

std::optional<std::string> HttpRequestReader::read_full_request() {
    if (!read_until_headers()) return std::nullopt;

    size_t header_end = buffer.find("\r\n\r\n");
    std::string headers = buffer.substr(0, header_end + 4);

    if (is_chunked(headers)) {
        if (!read_chunked_body()) return std::nullopt;
    } else {
        size_t content_length = get_content_length(headers);
        if (content_length > 0) {
            if (!read_fixed_length_body(content_length)) return std::nullopt;
        }
    }

    return buffer;
}

bool HttpRequestReader::read_until_headers() {
    char temp[buffer_size];
    while (buffer.find("\r\n\r\n") == std::string::npos) {
        ssize_t n = read(fd, temp, sizeof(temp));
        if (n <= 0) return false;
        buffer.append(temp, n);
    }
    return true;
}

size_t HttpRequestReader::get_content_length(const std::string& headers) {
    std::istringstream stream(headers);
    std::string line;
    while (std::getline(stream, line)) {
        std::string lower = line;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.starts_with("content-length:")) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                return std::stoul(line.substr(pos + 1));
            }
        }
    }
    return 0;
}

bool HttpRequestReader::is_chunked(const std::string& headers) {
    std::istringstream stream(headers);
    std::string line;
    while (std::getline(stream, line)) {
        std::string lower = line;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.starts_with("transfer-encoding:") &&
            lower.find("chunked") != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool HttpRequestReader::read_fixed_length_body(size_t length) {
    size_t header_end = buffer.find("\r\n\r\n") + 4;
    size_t current_body_size = buffer.size() - header_end;
    char temp[buffer_size];
    while (current_body_size < length) {
        ssize_t n = read(fd, temp, sizeof(temp));
        if (n <= 0) return false;
        buffer.append(temp, n);
        current_body_size += n;
    }
    return true;
}

bool HttpRequestReader::read_chunked_body() {
    char temp[buffer_size];

    while (true) {
        size_t pos;
        while ((pos = buffer.find("\r\n")) == std::string::npos) {
            ssize_t n = read(fd, temp, sizeof(temp));
            if (n <= 0) return false;
            buffer.append(temp, n);
        }

        std::string size_line = buffer.substr(0, pos);
        buffer.erase(0, pos + 2);

        size_t chunk_size = 0;
        try {
            chunk_size = std::stoul(size_line, nullptr, 16);
        } catch (...) {
            return false;
        }

        if (chunk_size == 0) {
            while (true) {
                while ((pos = buffer.find("\r\n")) == std::string::npos) {
                    ssize_t n = read(fd, temp, sizeof(temp));
                    if (n <= 0) return false;
                    buffer.append(temp, n);
                }

                std::string line = buffer.substr(0, pos);
                buffer.erase(0, pos + 2);
                if (line.empty()) break;
            }
            return true;
        }

        while (buffer.size() < chunk_size + 2) {
            ssize_t n = read(fd, temp, sizeof(temp));
            if (n <= 0) return false;
            buffer.append(temp, n);
        }

        buffer.erase(0, chunk_size);
        if (buffer.substr(0, 2) != "\r\n") return false;
        buffer.erase(0, 2);
    }
}
