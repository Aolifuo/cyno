#ifndef CYNO_HTTP_CLIENT_H_
#define CYNO_HTTP_CLIENT_H_

#include "asio/awaitable.hpp"
#include "asio/io_context.hpp"
#include "cyno/base/Pimpl.h"
#include "cyno/http/HttpMessage.h"

namespace cyno {

class HttpClient {

    CLASS_PIMPL_DECLARE(HttpClient)

public:
    // throw
    static asio::awaitable<HttpResponse> execute(std::string_view url, bool should_keep_alive = false);
    static asio::awaitable<HttpResponse> execute(HttpRequest req);
};

}


#endif