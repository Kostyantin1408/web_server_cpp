#ifndef HTTPREQUESTREADER_HPP
#define HTTPREQUESTREADER_HPP

#include <string>
#include <unistd.h>
#include <string_view>
#include <optional>

class HttpRequestReader {
public:
    explicit HttpRequestReader(int socket_fd);

    std::optional<std::string> read_full_request();

private:
    int fd;
    static constexpr size_t buffer_size = 1024;
    std::string buffer;

    bool read_until_headers();
    size_t get_content_length(const std::string& headers);
    bool is_chunked(const std::string& headers);
    bool read_fixed_length_body(size_t length);
    bool read_chunked_body();
};

#endif //HTTPREQUESTREADER_HPP
