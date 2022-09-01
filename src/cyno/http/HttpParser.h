#ifndef CYNO_HTTP_PARSER_H_
#define CYNO_HTTP_PARSER_H_

#include "cyno/base/Exceptions.h"
#include "cyno/http/HttpMessage.h"

namespace cyno {

template<typename T>
requires (std::is_same_v<T, HttpRequest> || std::is_same_v<T, HttpResponse>)
class HttpParser {
public:
    enum State {
        NotStarted, Parsing, Fail, Success
    };

    HttpParser() {
        settings_ = http_parser_settings{
            .on_message_begin = on_message_begin,
            .on_url = on_url,
            .on_status = on_status,
            .on_header_field = on_header_field,
            .on_header_value = on_header_value,
            .on_headers_complete = on_headers_complete,
            .on_body = on_body,
            .on_message_complete = on_message_complete,
        };

        http_parser_init(&parser_, http_parser_type::HTTP_BOTH);
        parser_.data = static_cast<void*>(this);
        state_ = NotStarted;
    }

    void restart() {
        result_ = T{};
        http_parser_init(&parser_, http_parser_type::HTTP_BOTH);
        parser_.data = static_cast<void*>(this);
        state_ = NotStarted;
    }

    // throw HttpRequestError
    void parse(std::string_view str) {
        http_parser_execute(&parser_, &settings_, str.data(), str.length());
        if (parser_.http_errno != http_errno::HPE_OK) {
            state_ = Fail;
            throw HttpRequestError(http_errno_description(HTTP_PARSER_ERRNO(&parser_)));
        }

        result_.should_keep_alive = http_should_keep_alive(&parser_);
        
        if constexpr (std::is_same_v<T, HttpRequest>) {
            // request
            result_.method = static_cast<http_method>(parser_.method);
        } else {
            // response
            result_.status_code = static_cast<http_status>(parser_.status_code);
        }
        
    }

    T& result() {
        return result_;
    }

    State state() {
        return state_;
    }

private:
    static HttpParser& hook(void * data) {
        return *static_cast<HttpParser*>(data);
    }

    static int on_message_begin(http_parser* parser) {
        hook(parser->data).state_ = Parsing;
        return 0;
    }

    static int on_url(http_parser* parser, const char* at, size_t len) {
        if constexpr (std::is_same_v<T, HttpRequest>) {
            hook(parser->data).result_.url.assign(at, len);
        }
        return 0;
    }

    static int on_status(http_parser* parser, const char* at, size_t len) {
        if constexpr (std::is_same_v<T, HttpResponse>) {
            hook(parser->data).result_.status.assign(at, len);
        }
        return 0;
    }

    static int on_header_field(http_parser* parser, const char* at, size_t len) {
        hook(parser->data).current_kv_.first.assign(at, len);
        return 0;
    }

    static int on_header_value(http_parser* parser, const char* at, size_t len) {
        hook(parser->data).current_kv_.second.assign(at, len);
        hook(parser->data).result_.headers.insert(std::move(hook(parser->data).current_kv_));
        return 0;
    }

    static int on_headers_complete(http_parser* parser) {
         hook(parser->data).result_.content_length = parser->content_length;
        return 0;
    }

    static int on_body(http_parser* parser, const char* at, size_t len) {
        hook(parser->data).result_.body.append(at,len);
        return 0;
    }

    static int on_message_complete(http_parser* parser) {
        hook(parser->data).state_ = Success;
        return 0;
    }

    http_parser parser_;
    http_parser_settings settings_;
    State state_;

    T result_;
    std::pair<std::string, std::string> current_kv_;
};

inline HttpRequestUrl parse_http_request_url(std::string_view url) {
    // /your/path?a=b&c=d
    HttpRequestUrl res;
    size_t pos = 0;
    for (; pos < url.length() && url[pos] != '?' && url[pos] == '/';) {
        std::string path;
        for (++pos; pos < url.length() && url[pos] != '/' && url[pos] != '?'; ++pos) {
            path.push_back(url[pos]);
        }
        if (!path.empty()) {
            res.path.push_back(std::move(path));
        }
    }

    if (pos >= url.length()) {
        return res;
    }

    for (++pos; pos < url.length(); ++pos) {
        std::string key;
        std::string value;
        for (; pos < url.length() && url[pos] != '='; ++pos) {
            key.push_back(url[pos]);
        }
        for (++pos; pos < url.length() && url[pos] != '&'; ++pos) {
            value.push_back(url[pos]);
        }
        res.query.emplace(std::move(key), std::move(value));
    }

    return res;
}

}

#endif