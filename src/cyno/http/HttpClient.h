#ifndef CYNO_HTTP_CLIENT_H_
#define CYNO_HTTP_CLIENT_H_

#include "asio/awaitable.hpp"
#include "asio/io_context.hpp"
#include "cyno/base/Pimpl.h"
#include "cyno/http/HttpMessage.h"
#include <string_view>

namespace cyno {

class HttpClient {

    CLASS_PIMPL_DECLARE(HttpClient)

public:
    // throw
    static asio::awaitable<HttpResponse> execute(std::string url);
    static asio::awaitable<HttpResponse> execute(const HttpRequest& req, std::string host, std::string service);
    static asio::awaitable<HttpResponse> execute(std::string req_str, std::string host, std::string service);
};

}


#endif