#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

template <typename T>
class ThreadSafeQueue {
public:
    void push(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopRequested_) return;
        queue_.push(value);
        cond_var_.notify_one();
    }

    bool tryPop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return false;
        value = queue_.front();
        queue_.pop();
        return true;
    }

    bool waitPop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_var_.wait(lock, [this] {
            return !queue_.empty() || stopRequested_;
        });

        if (stopRequested_ && queue_.empty()) return false;

        value = queue_.front();
        queue_.pop();
        return true;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopRequested_ = true;
        }
        cond_var_.notify_all();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) queue_.pop();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    bool isStopped() const {
        return stopRequested_;
    }

private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable cond_var_;
    std::atomic<bool> stopRequested_ = false;
};

#endif
