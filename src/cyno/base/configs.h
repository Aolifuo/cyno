#ifndef CYNO_CONFIGS_H_
#define CYNO_CONFIGS_H_

namespace cyno {

struct PoolConfig {
    size_t core_idle_size;
    size_t max_idle_size;
    size_t max_active_size;
    size_t max_idle_time; 
    size_t wait_timeout;
};

}

#endif