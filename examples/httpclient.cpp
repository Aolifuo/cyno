#include <exception>
#include "asio/co_spawn.hpp"
#include "asio/detached.hpp"
#include "spdlog/spdlog.h"
#include "cyno/http/HttpClient.h"

using namespace std;
using namespace cyno;

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