#ifndef CYNO_RESOURECEPOOL_H_
#define CYNO_RESOURECEPOOL_H_

#include <memory>
#include <memory_resource>
#include "asio/awaitable.hpp"
#include "asio/steady_timer.hpp"
#include "asio/use_awaitable.hpp"
#include "cyno/base/Exceptions.h"
#include "cyno/base/configs.h"

namespace cyno {

template<typename T, typename Executor = asio::any_io_executor>
class ResourcePool {
public:
    inline static std::pmr::unsynchronized_pool_resource pool_resource;

    using executor_type = Executor;

    struct Item {
        std::unique_ptr<T> data;
        std::chrono::steady_clock::time_point time_stamp = std::chrono::steady_clock::now();
    };

    ResourcePool(executor_type executor, PoolConfig config, std::function<T()> init)
        : executor_(executor)
        , config_(config)
        , initializer(std::move(init))
        , timer_(executor_)
        , idle_list_(&pool_resource)
    {
        for (size_t i = 0 ; i < config_.core_idle_size; ++i) {
            idle_list_.push_back({std::make_unique<T>(initializer())});
        }
        timer_.expires_after(std::chrono::milliseconds(config_.max_idle_time));
        run_scheduled_cleanup_task();
    }

    ResourcePool(ResourcePool &&) = delete;
    ResourcePool& operator=(ResourcePool &&) = delete;

    asio::awaitable<std::shared_ptr<T>> borrow() {
        std::unique_lock lk(mutex_);
        if (active_size_ >= config_.max_active_size) {
            lk.unlock();
            asio::steady_timer timer(executor_, 
                                    std::chrono::milliseconds(config_.wait_timeout));
            co_await timer.async_wait(asio::use_awaitable);
            lk.lock();

            if (active_size_ >= config_.max_active_size) {
                throw ResourceExhaustedError("Pool resource exhausted");
            }
        }

        ++active_size_;
        if (idle_list_.empty()) {
            co_return std::shared_ptr<T>(new T(initializer()), [this](T* elem) { return_item(elem); });
        }

        auto item = std::move(idle_list_.front());
        idle_list_.pop_front();
        co_return std::shared_ptr<T>(item.data.release(), [this](T* elem) { return_item(elem); });
    }

    void return_item(T* elem) {
        std::unique_lock lk(mutex_);
        --active_size_;
        if (idle_list_.size() >= config_.max_idle_size) {
            delete elem;
            return;
        }

        idle_list_.push_back({
            std::unique_ptr<T>(elem), 
            std::chrono::steady_clock::now() + std::chrono::milliseconds(config_.max_idle_time)
        });
    }

    executor_type get_executor() {
        return executor_;
    }
private:

    void run_scheduled_cleanup_task() {
        timer_.async_wait([this](std::error_code err) {
            if (err) {
                return;
            }

            if (idle_list_.size() > config_.core_idle_size) {
                std::unique_lock lk(mutex_);
                for (auto it = idle_list_.begin(); 
                    it != idle_list_.end() && idle_list_.size() > config_.core_idle_size;) 
                {
                    if (it->time_stamp <= std::chrono::steady_clock::now()) {
                        it = idle_list_.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            
            run_scheduled_cleanup_task();
        });
    }

    executor_type executor_;
    PoolConfig config_;
    std::function<T()> initializer;
    asio::steady_timer timer_;

    // idle = idle_list_.size()
    // active = active_size_
    // all = idle + active
    std::pmr::list<Item> idle_list_;
    size_t active_size_ = 0;
    std::mutex mutex_;
};

}

#endif