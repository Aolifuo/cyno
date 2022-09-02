#include <exception>
#include <iostream>
#include "asio/co_spawn.hpp"
#include "asio/detached.hpp"
#include "cyno/http/HttpClient.h"

using namespace std;
using namespace cyno;

asio::awaitable<void> amain() {
    try {
        auto resp = co_await HttpClient::execute("https://www.baidu.com");
        cout << resp.body << '\n';
    } catch(const std::exception& e) {
        cout << e.what() << '\n';
    }
}

int main() {
    asio::io_context ioc;
    asio::co_spawn(ioc, amain(), asio::detached);
    ioc.run();  
}