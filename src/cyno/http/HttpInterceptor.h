#ifndef CYNO_INTERCEPTOR_H_
#define CYNO_INTERCEPTOR_H_

#include "cyno/http/HttpMessage.h"

namespace cyno {

// Interface
class HttpInterceptor {
public:
    virtual bool before(HttpRequest&, HttpResponse&) = 0;
    virtual void after(HttpRequest&, HttpResponse&) = 0;
    virtual ~HttpInterceptor() = default;
};

}

#endif