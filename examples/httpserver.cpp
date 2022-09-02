#include "cyno/http/HttpRouter.h"
#include "cyno/http/HttpServer.h"

#include <iostream>

using namespace std;
using namespace cyno;

class MyExceptionHandler: public ExceptionHandler {
public:
    int handle(exception_ptr eptr, HttpResponse& resp) override {
        try {
            rethrow_exception(eptr);
        } catch(const exception& e) {
            resp.plain(e.what());
            cerr << e.what() << endl;
        }

        return 400;
    };
};

class MyInterceptor: public HttpInterceptor {
public:
    bool before(HttpRequest& req, HttpResponse& resp) override {
        cout << "before interceptor\n";
        return true;
    }

    void after(HttpRequest& req, HttpResponse& resp) override {
        cout << "after interceptor\n";
    }
};

HttpRouter my_router() {
    HttpRouter router;
    router.GET("/login", [](HttpRequest& req, HttpResponse& resp) {
        cout << "username" << req.query["username"] << " password" << req.query["password"] << '\n';

        return resp.plain("login succeeded");
    });

    router.POST("/info/*", [](HttpRequest& req, HttpResponse& resp) {
        for (auto& token : req.path) {
            cout << token << " ";
        }
        cout << "\n" << req.body << '\n';

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