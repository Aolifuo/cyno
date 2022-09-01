#include "cyno/http/HttpRouter.h"
#include "cyno/http/HttpServer.h"

#include <cstdio>

using namespace std;
using namespace cyno;

class MyExceptionHandler: public ExceptionHandler {
public:
    int handle(exception_ptr eptr, HttpResponse& resp) override {
        try {
            rethrow_exception(eptr);
        } catch(const exception& e) {
            resp.plain(e.what());
            std::printf("error: %s\n", e.what());
        }

        return 400;
    };
};

class MyInterceptor: public HttpInterceptor {
public:
    bool before(HttpRequest& req, HttpResponse& resp) override {
        std::printf("before interceptor\n");
        return true;
    }

    void after(HttpRequest& req, HttpResponse& resp) override {
        std::printf("after interceptor\n");
    }
};

HttpRouter my_router() {
    HttpRouter router;
    router.GET("/login", [](HttpRequest& req, HttpResponse& resp) {
        std::printf("username: %s, password: %s\n", 
                    req.query["username"].c_str(), req.query["password"].c_str());

        return resp.plain("login succeeded");
    });

    router.POST("/info/*", [](HttpRequest& req, HttpResponse& resp) {
        std::printf("body:\n %s\n", req.body.c_str());
        // resp.headers["Connection"] = "close";
        return 200;
    });

    return router;
}

int main() {
    try {
        asio::io_context ioc;

        HttpServer server(ioc);
        server.routes(my_router());
        server.add_interceptor(make_unique<MyInterceptor>());
        server.exception_handler(make_unique<MyExceptionHandler>());
        server.bind("[::1]", 8080);
        server.start();

        ioc.run();
    } catch(const exception& e) {
        std::printf("%s\n", e.what());
    }
}