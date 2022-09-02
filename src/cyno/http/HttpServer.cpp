#include "cyno/http/HttpRouter.h"
#include "cyno/http/HttpServer.h"

#include <iostream>
#include "asio/ip/tcp.hpp"
#include "asio/co_spawn.hpp"
#include "asio/detached.hpp"
#include "asio/awaitable.hpp"
#include "asio/read_until.hpp"
#include "asio/write.hpp"
#include "asio/steady_timer.hpp"
#include "asio/experimental/awaitable_operators.hpp"

#include "cyno/http/HttpParser.h"

namespace cyno {

using namespace std::chrono_literals;
using namespace asio::experimental::awaitable_operators;

struct HttpServer::Impl {
    asio::io_context& io_context;
    asio::ip::tcp::acceptor acceptor;

    HttpRouter router;
    std::unique_ptr<ExceptionHandler> exception_handler;
    std::vector<std::unique_ptr<HttpInterceptor>> interceptors;

    asio::awaitable<void> run_accept();
    asio::awaitable<void> process(asio::ip::tcp::socket socket);

    void dispatch(HttpRequest&, HttpResponse&);
};

void perfect_response(HttpResponse&, int status);

CLASS_PIMPL_IMPLEMENT(HttpServer)

HttpServer::HttpServer(asio::io_context& ioc) {
    impl = new Impl{ioc, asio::ip::tcp::acceptor{ioc}};
}

void HttpServer::bind(std::string_view host, unsigned port) {
    asio::ip::tcp::endpoint ep(asio::ip::make_address(host), port);
    impl->acceptor = asio::ip::tcp::acceptor(impl->io_context, ep);
}

void HttpServer::routes(HttpRouter router) {
    impl->router = std::move(router);
}

void HttpServer::add_interceptor(std::unique_ptr<HttpInterceptor> interceptor) {
    impl->interceptors.emplace_back(std::move(interceptor));
}

void HttpServer::exception_handler(std::unique_ptr<ExceptionHandler> handler) {
    impl->exception_handler = std::move(handler);
}

void HttpServer::start() {
    asio::co_spawn(impl->io_context, impl->run_accept(), asio::detached);
}

void HttpServer::stop() {
    
}

asio::awaitable<void> HttpServer::Impl::run_accept() {
    try {
        for (; ;) {
            asio::ip::tcp::socket socket(acceptor.get_executor());

            co_await acceptor.async_accept(socket, asio::use_awaitable);

            asio::co_spawn(acceptor.get_executor(), 
                [this, sock = std::move(socket)]() mutable 
                {
                    return process(std::move(sock));
                },
                asio::detached);
        }
    } catch(const std::system_error& e) {
        std::printf("system error happened in run_accept: %s\n", e.what());
    }
}

asio::awaitable<void> HttpServer::Impl::process(asio::ip::tcp::socket socket) {
    // 
    HttpParser<HttpRequest> parser;
    HttpRequest& req = parser.result();
    HttpResponse resp = HttpResponse::from_default();

    std::string buffer;
    buffer.reserve(4 * 1024);

    try {
        for (; ;) {
            buffer.clear();
            //timer.expires_after(5s);
            // read until \r\n\r\n
            co_await (asio::async_read_until(socket, asio::dynamic_buffer(buffer),
                                                "\r\n\r\n", asio::use_awaitable)
                        //|| timer.async_wait(asio::use_awaitable)
                        );
            try {
                // parse first line and headers
                parser.parse({buffer.data(), buffer.size()});

                // read body
                buffer.resize(4 * 1024);
                for (; parser.state() != HttpParser<HttpRequest>::Success; ) {
                    size_t read_len = co_await socket.async_read_some(asio::buffer(buffer),
                                                                        asio::use_awaitable);
                    parser.parse({buffer.data(), read_len});
                }
                
                // dispatch
                auto url = parse_http_request_url(req.url);
                req.path = std::move(url.path);
                req.query = std::move(url.query);
                dispatch(req, resp);

            } catch(...) {
                if (exception_handler) {
                    int status = exception_handler->handle(std::current_exception(), resp);
                    perfect_response(resp, status);
                } else {
                    break;
                }
            }

            buffer.assign(serialize_response(resp));
            // send
            co_await asio::async_write(socket, asio::buffer(buffer), 
                                            asio::use_awaitable);
            
            // keepalive
            if (!req.should_keep_alive) {
                break;
            }
        }
    } catch(const std::system_error& err) {
        //
    }
}

void HttpServer::Impl::dispatch(HttpRequest& req, HttpResponse& resp) {
    // interceptor
    for (auto& aop : interceptors) {
        if (!aop->before(req, resp)) {
            throw InterceptorError("The conditions for passing the interceptor are not met");
        }
    }

    // router
    int status = router.match(http_method_str(req.method), req.path)(req, resp);
    perfect_response(resp, status);

    // interceptor
    for (auto it = interceptors.rbegin(); it != interceptors.rend(); ++it) {
        (*it)->after(req, resp);
    }

}

void perfect_response(HttpResponse& resp, int status) {
    resp.status_code = static_cast<HttpResponse::Status>(status);
    resp.status = http_status_str(resp.status_code);

    resp.content_length = resp.body.length();
    resp.headers["Content-Length"] = std::to_string(resp.content_length);
}


}