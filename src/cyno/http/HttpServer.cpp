#include "cyno/http/HttpRouter.h"
#include "cyno/http/HttpServer.h"

#include "spdlog/spdlog.h"
#include "asio/ip/tcp.hpp"
#include "asio/co_spawn.hpp"
#include "asio/detached.hpp"
#include "asio/awaitable.hpp"
#include "asio/read_until.hpp"
#include "asio/write.hpp"
#include "asio/steady_timer.hpp"

#include "cyno/base/ResourcePool.h"
#include "cyno/http/HttpParser.h"
#include "cyno/http/HttpConfig.h"

namespace cyno {

using namespace std::chrono_literals;

struct HttpServer::Impl {
    inline static std::pmr::synchronized_pool_resource sync_pool;

    State state;
    asio::any_io_executor executor;
    asio::ip::tcp::acceptor acceptor;

    HttpRouter router;
    std::unique_ptr<ExceptionHandler> exception_handler;
    std::vector<std::unique_ptr<HttpInterceptor>> interceptors;

    ResourcePool<std::pmr::string> buffer_resource{
        executor, 
        PoolConfig(http_config.buffer_pool_config),
        []{
            std::pmr::string str(&ResourcePool<std::pmr::string>::pool_resource);
            str.reserve(http_config.request_config.max_line_and_headers_size);
            return str;
        }};

    asio::awaitable<void> run_accept();
    asio::awaitable<void> process(asio::ip::tcp::socket socket);
    void dispatch(HttpRequest&, HttpResponse&);
};

void perfect_response(HttpResponse&, int status);

CLASS_PIMPL_IMPLEMENT(HttpServer)

HttpServer::HttpServer(asio::any_io_executor executor) {
    impl = new Impl{Stopped, executor, asio::ip::tcp::acceptor{executor}};
}

void HttpServer::bind(std::string_view host, unsigned port) {
    asio::ip::tcp::endpoint ep(asio::ip::make_address(host), port);
    impl->acceptor = asio::ip::tcp::acceptor(impl->executor, ep);
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
    impl->state = Running;
    asio::co_spawn(impl->executor, impl->run_accept(), asio::detached);
}

void HttpServer::stop() {
    impl->state = Stopped;
}

asio::awaitable<void> HttpServer::Impl::run_accept() {
    try {
        for (; state == Running;) {
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
        spdlog::error("System error happened in run_accept: {}\n", e.what());
    }
}

asio::awaitable<void> HttpServer::Impl::process(asio::ip::tcp::socket socket) {
    // 
    HttpParser<HttpRequest> parser;
    HttpRequest& req = parser.result();
    HttpResponse resp = HttpResponse::from_default();
    asio::steady_timer deadline(executor);

    auto check_deadline = [&socket, &deadline] {
        deadline.async_wait([&socket](asio::error_code err) {
            if (!err) {
                socket.close();
            }
        });
    };
    
    try {
        // borrow buffer
        auto buffer = co_await buffer_resource.borrow();

        for (; ;) {
            buffer->clear();
            
            // set timeout
            if (req.should_keep_alive) {
                deadline.expires_after(
                    std::chrono::milliseconds(http_config.request_config.keepalive_timeout));
            } else {
                deadline.expires_after(
                    std::chrono::milliseconds(http_config.request_config.receive_headers_timeout));
            }
            
            check_deadline();

            // read until \r\n\r\n
            co_await asio::async_read_until(socket, asio::dynamic_buffer(*buffer),
                                            "\r\n\r\n", asio::use_awaitable);
            
            // refresh deadline and set for receive body task
            deadline.expires_after(
                std::chrono::milliseconds(http_config.request_config.receive_body_timeout));
            try {
                // parse first line and headers
                parser.parse({buffer->data(), buffer->size()});

                buffer->resize(http_config.request_config.max_line_and_headers_size);

                // read body
                for (; parser.state() != HttpParser<HttpRequest>::Success; ) {
                    check_deadline();
                    size_t read_len = co_await socket.async_read_some(asio::buffer(*buffer),
                                                                        asio::use_awaitable);
                    deadline.cancel_one();
                    parser.parse({buffer->data(), read_len});
                }

                // dispatch
                auto url = parse_http_request_url(req.url);
                req.path = std::move(url.path);
                req.query = std::move(url.query);
                dispatch(req, resp);

            } catch(const CynoRuntimeError& err) {
                if (exception_handler) {
                    int status = exception_handler->handle(std::current_exception(), resp);
                    perfect_response(resp, status);
                } else {
                    break;
                }
            }

            buffer->assign(serialize_response(resp));

            // send
            co_await asio::async_write(socket, asio::buffer(*buffer), 
                                            asio::use_awaitable);
            
            // keepalive
            if (!req.should_keep_alive) {
                break;
            }
        }
    } catch(const std::system_error& err) {
        spdlog::error("System error happend when receiving or sending: {}", err.what());
    }

    // return buffer
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