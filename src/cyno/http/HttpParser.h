#ifndef CYNO_HTTP_PARSER_H_
#define CYNO_HTTP_PARSER_H_

#include "cyno/base/Exceptions.h"
#include "cyno/http/HttpMessage.h"
#include "http_parser.h"

namespace cyno {

class HttpRequestParser {
public:
    enum State {
        NotStarted, Parsing, Fail, Success
    };

    HttpRequestParser() {
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

        http_parser_init(&parser_, http_parser_type::HTTP_REQUEST);
        parser_.data = static_cast<void*>(this);
        state_ = NotStarted;
    }

    void restart() {
        req_ = HttpRequest{};
        http_parser_init(&parser_, http_parser_type::HTTP_REQUEST);
        parser_.data = static_cast<void*>(this);
        state_ = NotStarted;
    }

    // throw HttpRequestError
    void parse(std::string_view request) {
        http_parser_execute(&parser_, &settings_, request.data(), request.length());
        if (parser_.http_errno != http_errno::HPE_OK) {
            state_ = Fail;
            throw HttpRequestError(http_errno_description(HTTP_PARSER_ERRNO(&parser_)));
        }

        req_.method = static_cast<http_method>(parser_.method);
        req_.should_keep_alive = http_should_keep_alive(&parser_);
    }

    HttpRequest& result() {
        return req_;
    }

    State state() {
        return state_;
    }

private:
    static HttpRequestParser& hook(void * data) {
        return *static_cast<HttpRequestParser*>(data);
    }

    static int on_message_begin(http_parser* parser) {
        hook(parser->data).state_ = Parsing;
        return 0;
    }

    static int on_url(http_parser* parser, const char* at, size_t len) {
        hook(parser->data).req_.url.assign(at, len);
        return 0;
    }

    static int on_status(http_parser*, const char* at, size_t len) {
        
        return 0;
    }

    static int on_header_field(http_parser* parser, const char* at, size_t len) {
        hook(parser->data).current_kv_.first.assign(at, len);
        return 0;
    }

    static int on_header_value(http_parser* parser, const char* at, size_t len) {
        hook(parser->data).current_kv_.second.assign(at, len);
        hook(parser->data).req_.headers.insert(std::move(hook(parser->data).current_kv_));
        return 0;
    }

    static int on_headers_complete(http_parser* parser) {
         hook(parser->data).req_.content_length = parser->content_length;
        return 0;
    }

    static int on_body(http_parser* parser, const char* at, size_t len) {
        hook(parser->data).req_.body.append(at,len);
        return 0;
    }

    static int on_message_complete(http_parser* parser) {
        hook(parser->data).state_ = Success;
        return 0;
    }

    http_parser parser_;
    http_parser_settings settings_;
    State state_;

    HttpRequest req_;
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

inline size_t write_response_to_buffer(HttpResponse& resp, std::string& buffer) {
    size_t len = resp.version.length() + 1 + 3 + 1 + resp.status.length() + 2;
    buffer.append(resp.version);
    buffer.append(" ");
    buffer.append(std::to_string(resp.status_code));
    buffer.append(" ");
    buffer.append(resp.status);
    buffer.append("\r\n");
    for (auto&& [key, value] : resp.headers) {
        len += key.length() + 2 + value.length() + 2;
        buffer.append(key);
        buffer.append(": ");
        buffer.append(value);
        buffer.append("\r\n");
    }
    len += 2;
    buffer.append("\r\n");
    len += resp.body.length();
    buffer.append(resp.body, 0, resp.content_length);
    return len;
}

}

#endif