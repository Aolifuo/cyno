#ifndef CYNO_HTTP_ROUTER_H_
#define CYNO_HTTP_ROUTER_H_

#include <functional>
#include "cyno/base/Pimpl.h"
#include "cyno/http/HttpMessage.h"

namespace cyno {

class HttpRouter {

    CLASS_PIMPL_DECLARE(HttpRouter)

public:
    HttpRouter();
    using HttpHandler = std::function<int(HttpRequest&, HttpResponse&)>;

    void GET(std::string_view path, HttpHandler handler);
    void POST(std::string_view path, HttpHandler handler);
    void PUT(std::string_view path, HttpHandler handler);
    void DELETE(std::string_view path, HttpHandler handler);

    HttpHandler& match(std::string_view method, const std::vector<std::string>& path);
private:
    // throw
    void insert_handler(std::string_view method, std::string_view path, HttpHandler handler);
};

}



#endif