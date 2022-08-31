#ifndef CYNO_EXCEPTION_HANDLER_H_
#define CYNO_EXCEPTION_HANDLER_H_

#include <exception>

namespace cyno {

// Interface
class ExceptionHandler {
public:
    virtual void handle(std::exception_ptr eptr) = 0;
    virtual ~ExceptionHandler() = default;
};

}

#endif