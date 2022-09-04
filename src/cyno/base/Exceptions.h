#ifndef CYNO_EXCEPTIONS_H_
#define CYNO_EXCEPTIONS_H_

#include <stdexcept>

namespace cyno {

class CynoRuntimeError: public std::runtime_error {
    using Base = std::runtime_error;
public:
    using Base::Base;
};

class HttpRequestError: public CynoRuntimeError {
    using Base = CynoRuntimeError;
public:
    using Base::Base;
};

class HttpResponseError:public CynoRuntimeError {
    using Base = CynoRuntimeError;
public:
    using Base::Base;
};

class IllegalUrlError: public CynoRuntimeError {
    using Base = CynoRuntimeError;
public:
    using Base::Base;
};

class IllegalRouteError: public CynoRuntimeError {
    using Base = CynoRuntimeError;
public:
    using Base::Base;
};

class NotMatchPathError: public CynoRuntimeError {
    using Base = CynoRuntimeError;
public:
    using Base::Base;
};

class InterceptorError: public CynoRuntimeError {
    using Base = CynoRuntimeError;
public:
    using Base::Base;
};

class TimeoutError: public CynoRuntimeError {
    using Base = CynoRuntimeError;
public:
    using Base::Base;
};

class ResourceExhaustedError: public CynoRuntimeError {
    using Base = CynoRuntimeError;
public:
    using Base::Base;
};


}



#endif