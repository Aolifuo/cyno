# cyno

cyno 是一个基于asio的http网络库

## Http server

```cpp
class MyExceptionHandler: public ExceptionHandler {
public:
    int handle(exception_ptr eptr, HttpResponse& resp) override {
        try {
            rethrow_exception(eptr);
        } catch(const exception& e) {
            resp.plain(e.what());
            spdlog::warn("{}", e.what());
        }

        return 400;
    };
};

class MyInterceptor: public HttpInterceptor {
public:
    bool before(HttpRequest& req, HttpResponse& resp) override {
        spdlog::info("before interceptor");
        return true;
    }

    void after(HttpRequest& req, HttpResponse& resp) override {
        spdlog::info("before interceptor");
    }
};

HttpRouter my_router() {
    HttpRouter router;
    router.Get("/login", [](HttpRequest& req, HttpResponse& resp) {
        spdlog::info("username: {} password: {}", req.query["username"], req.query["password"]);

        return resp.plain("login succeeded");
    });

    router.Post("/info/*", [](HttpRequest& req, HttpResponse& resp) {
        spdlog::info("{}", req.body);

        return 200;
    });

    return router;
}

int main() {
    try {
        spdlog::set_pattern("[%C-%m-%d %H:%M:%S] [%^%l%$] %v");

        asio::io_context ioc;

        HttpServer server(ioc.get_executor());
        server.routes(my_router());
        server.add_interceptor(make_unique<MyInterceptor>());
        server.exception_handler(make_unique<MyExceptionHandler>());
        server.bind("127.0.0.1", 8080);
        server.start();

        ioc.run();
    } catch(const exception& e) {
        spdlog::critical("{}", e.what());
    }
}
```

## Http client

```cpp
asio::awaitable<void> amain() {
    try {
        auto resp = co_await HttpClient::execute("https://www.baidu.com");
        
        spdlog::info("{}", resp.body);
    } catch(const std::exception& e) {
        spdlog::error("{}", e.what());
    }
}

int main() {
    asio::io_context ioc;
    asio::co_spawn(ioc, amain(), asio::detached);
    ioc.run();  
}
```
