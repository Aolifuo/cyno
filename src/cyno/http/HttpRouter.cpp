#include "cyno/http/HttpRouter.h"

#include <map>
#include <cstdio>
#include "cyno/base/Exceptions.h"

namespace cyno {

struct RouteParser {
    enum TokenType {
        RoutePart, Asterisk, Slash
    };

    RouteParser(std::string_view path) {
        for (auto it = path.begin(); it != path.end();) {
            if (*it == '/') {
                tokens.push_back({Slash, "/"});
                ++it;
                continue;
            }

            if (*it == '*') {
                tokens.push_back({Asterisk ,"*"});
                if (++it != path.end()) {
                    throw IllegalRouteError("The symbol '*' is not at the end of the routing path");
                }
                break;
            }

            auto tail = it + 1;
            for (; tail != path.end() && *tail != '/' && *tail != '*'; ++tail);
            tokens.push_back({RoutePart, {it, tail}});
            it = tail;
        }

        current = tokens.begin();
    }

    bool start() {
        if (current->first == Asterisk) {
            return check_and_next("*");
        }

        if (current->first == Slash) {
            return check_and_next("/") && parse_first() && parse_tail();
        }

        return false;
    }

    bool parse_first() {
        if (current == tokens.end()) {
            return true;
        }

        if (current->first == RoutePart) {
            return parse_part() && parse_second();
        }

        return true;
    }

    bool parse_second() {
        if (current == tokens.end()) {
            return true;
        }

        if (current->first == Slash) {
            return check_and_next("/") && parse_part() && parse_second();
        }

        return true;
    }

    bool parse_tail() {
        if (current == tokens.end()) {
            return true;
        }

        if (current->first == Asterisk) {
            return check_and_next("*");
        }

        return true;
    }

    bool parse_part() {
        ++current;
        return true;
    }

    bool check_and_next(const char* str) {
        if (current->second == str) {
            ++current;
            return true;
        }
        return false;
    }

    std::vector<std::pair<TokenType, std::string_view>> tokens;
    std::vector<std::pair<TokenType, std::string_view>>::iterator current;
};

struct FuzzyComparable {
    bool operator()(std::string_view left, std::string_view right) const {
        return left.length() < right.length();
    }
};

struct HttpRouter::Impl{
    std::unordered_map<std::string, HttpHandler> exact_map;
    std::map<std::string, HttpHandler, FuzzyComparable> fuzzy_map;
};

CLASS_PIMPL_IMPLEMENT(HttpRouter)

HttpRouter::HttpRouter() {
    impl = new Impl;
}

void HttpRouter::GET(std::string_view path, HttpHandler handler) {
    insert_handler("GET", path, handler);
}

void HttpRouter::POST(std::string_view path, HttpHandler handler) {
    insert_handler("POST", path, handler);
}

void HttpRouter::PUT(std::string_view path, HttpHandler handler) {
    insert_handler("PUT", path, handler);
}

void HttpRouter::DELETE(std::string_view path, HttpHandler handler) {
    insert_handler("DELETE", path, handler);
}

void HttpRouter::insert_handler(std::string_view method, std::string_view path, HttpHandler handler) {
    if (path.empty()) {
        throw IllegalRouteError("Route cannot be empty");
    }

    RouteParser parser(path);
    if (!parser.start()) {
        throw IllegalRouteError("Route syntax error");
    }

    if (path.ends_with("*")) {
        impl->fuzzy_map.emplace(std::string(method).append(path), std::move(handler));
    } else {
        impl->exact_map.emplace(std::string(method).append(path), std::move(handler));
    }
}

HttpRouter::HttpHandler& HttpRouter::match(std::string_view method, const std::vector<std::string>& path) {
    std::string join_path(method);
    for (auto& str : path) {
        join_path.append("/");
        join_path.append(str);
    }

    // std::printf("join path: %s\n", join_path.c_str());

    if (auto it = impl->exact_map.find(join_path); it != impl->exact_map.end()) {
        return it->second;
    }

    for (auto&& [route, handler] : impl->fuzzy_map) {
        if (route.length() > join_path.length()) {
            break;
        }

        if (route.compare(0, route.length() - 1, join_path, 0, route.length() - 1) == 0) {
            return handler;
        }
    }

    throw NotMatchPathError("The requested path did not find a match in the routes");  
}

}