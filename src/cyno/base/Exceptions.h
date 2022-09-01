#ifndef CYNO_EXCEPTIONS_H_
#define CYNO_EXCEPTIONS_H_

#include <stdexcept>

namespace cyno {

class HttpRequestError: public std::runtime_error {
    using Base = std::runtime_error;
public:
    using Base::Base;
};

class HttpResponseError:public std::runtime_error {
    using Base = std::runtime_error;
public:
    using Base::Base;
};

class IllegalUrlError: public std::runtime_error {
    using Base = std::runtime_error;
public:
    using Base::Base;
};

class IllegalRouteError: public std::runtime_error {
    using Base = std::runtime_error;
public:
    using Base::Base;
};

class NotMatchPathError: public std::runtime_error {
    using Base = std::runtime_error;
public:
    using Base::Base;
};

class InterceptorError: public std::runtime_error {
    using Base = std::runtime_error;
public:
    using Base::Base;
};

class TimeoutError: public std::runtime_error {
    using Base = std::runtime_error;
public:
    using Base::Base;
};



}



#endif