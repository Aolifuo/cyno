#ifndef CYNO_HTTP_SERVER_H_
#define CYNO_HTTP_SERVER_H_

#include <memory>
#include "asio/any_io_executor.hpp"
#include "cyno/base/Pimpl.h"
#include "cyno/http/ExceptionHandler.h"
#include "cyno/http/HttpInterceptor.h"

namespace cyno {

class HttpRouter;

class HttpServer {

    CLASS_PIMPL_DECLARE(HttpServer)

public:
    HttpServer(asio::any_io_executor executor);

    void bind(std::string_view host, unsigned port);
    void routes(HttpRouter router);
    void exception_handler(std::unique_ptr<ExceptionHandler>  handler);
    void add_interceptor(std::unique_ptr<HttpInterceptor> interceptor);
    void buffer_provider();
    void start();
    void stop();
private:
};

}

#endif