#ifndef CYNO_EXCEPTION_HANDLER_H_
#define CYNO_EXCEPTION_HANDLER_H_

#include <exception>
#include "cyno/http/HttpMessage.h"

namespace cyno {

// Interface
class ExceptionHandler {
public:
    virtual int handle(std::exception_ptr eptr, HttpResponse& resp) = 0;
    virtual ~ExceptionHandler() = default;
};

}

#endif