#include "cyno/http/HttpClient.h"

#include <iostream>
#include "asio/ip/address_v6.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/write.hpp"
#include "asio/connect.hpp"
#include "asio/use_awaitable.hpp"

#include "cyno/http/HttpParser.h"

namespace cyno {

struct HttpClient::Impl {

};

CLASS_PIMPL_IMPLEMENT(HttpClient)

asio::awaitable<HttpResponse> HttpClient::execute(std::string url) {
    http_parser_url parser;
    http_parser_url_init(&parser);

    int state = http_parser_parse_url(url.data(), url.length(), 0, &parser);
    if (0 != state) {
        throw IllegalUrlError("Illegal url");
    }

    auto req = HttpRequest::from_default();
    std::string schema;
    std::string host;
    unsigned port = 0;

    if (parser.field_set & (1 << UF_SCHEMA)) {
        schema.assign(url.data() + parser.field_data[UF_SCHEMA].off,  parser.field_data[UF_SCHEMA].len);
    } else {
        throw IllegalUrlError("Url missing schema");
    }

    if (parser.field_set & (1 << UF_HOST)) {
        host.assign(url.data() + parser.field_data[UF_HOST].off, parser.field_data[UF_HOST].len);
        req.headers["Host"] = host;
    } else {
        throw IllegalUrlError("Url missing host");
    }

    if (parser.field_set & (1 << UF_PATH)) {
        req.url.assign(url.data() + parser.field_data[UF_PATH].off, parser.field_data[UF_PATH].len);
    }

    if (parser.field_set & (1 << UF_QUERY)) {
        req.url.append("?");
        req.url.append(url.data() + parser.field_data[UF_QUERY].off, parser.field_data[UF_QUERY].len);
    }

    if (parser.field_set & (1 << UF_PORT)) {
        port = parser.port;
    }

    std::string service = port != 0 ? std::to_string(port) : schema;

    return execute(req, std::move(host), std::move(service));
}

asio::awaitable<HttpResponse> HttpClient::execute(const HttpRequest& req, std::string host, std::string service) {
    return execute(serialize_request(req), std::move(host), std::move(service));
}

asio::awaitable<HttpResponse> HttpClient::execute(std::string req_str, std::string host, std::string service) {
    auto executor = co_await asio::this_coro::executor;
    asio::ip::tcp::resolver resolver(executor);
    auto eps = co_await resolver.async_resolve(host, service, asio::use_awaitable);

    asio::ip::tcp::socket socket(executor);
    co_await asio::async_connect(socket, eps, asio::use_awaitable);
    co_await asio::async_write(socket, asio::buffer(req_str), asio::use_awaitable);

    HttpParser<HttpResponse> response_parser;
    std::string buffer(4 * 1024, 0);

    for (; response_parser.state() != HttpParser<HttpResponse>::Success; ) {
        size_t len = co_await socket.async_receive(asio::buffer(buffer), asio::use_awaitable);
        
        response_parser.parse({buffer.data(), len});
    }

    co_return std::move(response_parser.result());
}

}