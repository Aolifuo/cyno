#ifndef CYNO_HTTP_MESSAGE_H_
#define CYNO_HTTP_MESSAGE_H_

#include <string>
#include <unordered_map>
#include "http_parser.h"
#include "cyno/base/Exceptions.h"

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
    bool should_keep_alive = true;
};

struct HttpResponse {
    using Status = http_status;
    Status status_code;
    std::string version;
    std::string status;
    std::string body;
    std::unordered_map<std::string, std::string> headers;

    size_t content_length = 0;
    bool should_keep_alive = true;

    int data(std::string_view type, std::string_view content) {
        body = content;
        headers["Content-Type"] = type;
        return Status::HTTP_STATUS_OK;
    }

    int plain(std::string_view content) {
        return data("text/plain; charset=UTF-8", content);
    }

};

inline std::string serialize_request(const HttpRequest& req) {
   std::string res;
    res.append(http_method_str(req.method));
    res.append(" ");
    res.append(req.url);
    res.append(" ");
    res.append(req.version);
    res.append("\r\n");
    for (auto&& [key, value] : req.headers) {
        res.append(key);
        res.append(": ");
        res.append(value);
        res.append("\r\n");
    }
    res.append("\r\n");
    res.append(req.body);

    return res;
}

inline std::string serialize_response(const HttpResponse& resp) {
    std::string res;
    res.append(resp.version);
    res.append(" ");
    res.append(std::to_string(resp.status_code));
    res.append(" ");
    res.append(resp.status);
    res.append("\r\n");
    for (auto&& [key, value] : resp.headers) {
        res.append(key);
        res.append(": ");
        res.append(value);
        res.append("\r\n");
    }
    res.append("\r\n");
    res.append(resp.body);

    return res;
}

}

#endif