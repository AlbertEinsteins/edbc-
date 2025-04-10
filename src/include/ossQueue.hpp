#ifndef _OSSQUEUE_HPP
#define _OSSQUEUE_HPP

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "import.hpp"
#include "common.hpp"

namespace edb {

template <typename T>
class ossQueue
{
private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;

public:
    unsigned int size() const
    {
        std::lock_guard<std::mutex> lk(mutex_);
        return queue_.size();
    }


    void push(const T& data)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        queue_.push(data);
        cv_.notify_one();
    }

    bool empty() const 
    {
        std::lock_guard<std::mutex> lk(mutex_);
        return queue_.empty();
    }

    bool try_pop(T& data)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if (queue_.empty()) {
            return false;
        }
        data = queue_.front();
        queue_.pop();
        return true;
    }

    void waitAndPop(T& data)
    {
        std::unique_lock<std::mutex> lk(mutex_);
        while (queue_.empty()) {
            cv_.wait(lk);
        }
        data = queue_.front();
        queue_.pop();
    }

    bool waitForPopMillis(T& data, long long millis)
    {
        std::unique_lock<std::mutex> lk(mutex_);
        while (queue_.empty()) {
            auto status = cv_.wait_for(lk, std::chrono::milliseconds(millis));
            if (std::cv_status::timeout == status) {
                return false;
            }
        }

        data = queue_.front();
        queue_.pop();
        return true;
    }
};


}

#endif