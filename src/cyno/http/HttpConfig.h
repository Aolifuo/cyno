#ifndef CYNO_HTTP_CONFIG_H_
#define CYNO_HTTP_CONFIG_H_

#include "cyno/base/configs.h"

namespace cyno {

struct RequestConfig {
    size_t max_line_and_headers_size;
    size_t max_body_size;
    size_t max_file_size;
    size_t receive_headers_timeout;
    size_t receive_body_timeout;
    size_t keepalive_timeout;
};

struct HttpConfig {

    /* buffer resource */
    PoolConfig buffer_pool_config
    {
        .core_idle_size = 100,
        .max_idle_size = 300,
        .max_active_size = 500,
        .max_idle_time = 1000 * 60,
        .wait_timeout = 1000 * 15
    };

    /* http request */
    RequestConfig request_config
    {
        .max_line_and_headers_size = 1024 * 4,
        .max_body_size = 1024 * 1024,
        .max_file_size = size_t(1024) * 1024 * 1024,
        .receive_headers_timeout = 1000 * 10,
        .receive_body_timeout = 1000 * 15,
        .keepalive_timeout = 1000 * 60
    };
};

inline HttpConfig http_config;

}

#endif