#ifndef CYNO_HTTP_MESSAGE_H_
#define CYNO_HTTP_MESSAGE_H_

#include <string>
#include <unordered_map>
#include "http_parser.h"

namespace cyno {

struct HttpRequestUrl {
    std::vector<std::string> path;
    std::unordered_map<std::string, std::string> query;
};

struct HttpRequest {
    using Method = http_method;
    Method method;
    std::string url;
    std::string version;
    std::string body;
    std::unordered_map<std::string, std::string> headers;

    std::vector<std::string> path;
    std::unordered_map<std::string, std::string> query;

    size_t content_length = 0;
    bool should_keep_alive = false;
};

struct HttpResponse {
    using Status = http_status;
    Status status_code;
    std::string version;
    std::string status;
    std::string body;
    std::unordered_map<std::string, std::string> headers;

    size_t content_length = 0;

    int data(std::string_view type, std::string_view content) {
        body = content;
        headers["Content-Type"] = type;
        return Status::HTTP_STATUS_OK;
    }

    int plain(std::string_view content) {
        return data("text/plain; charset=UTF-8", content);
    }

};

}

#endif