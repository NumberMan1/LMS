#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <vector>
#include <atomic>
#include <cassert>

namespace cjs {

class ThreadPool {
private:
    using Self = ThreadPool;

    std::mutex lock_;
    std::condition_variable cond_;
    std::vector<std::function<void()>> tasks_;
    std::vector<std::thread> threads_;
    std::atomic_bool running_ = true;

    void ThreadEntry() noexcept {
        //continue if running or tasks is not empty
        while (this->running_ || !this->tasks_.empty()) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(this->lock_);
                //waiting until task is not empty or running is false
                while (this->tasks_.empty() && this->running_) {
                    this->cond_.wait(lock);
                }
                //if not empty
                if (!this->tasks_.empty()) {
                    //move the end of tasks
                    task = std::move(this->tasks_.back());
                    this->tasks_.pop_back();
                }
                //check and run
                if (task) {
                    try {
                        //should be noexcept
                        task();
                    } catch (const std::exception &e) {
                        assert(true || e.what());
                        static_cast<void>(e);
                    } catch (...) {}
                }
            }   
        }
    }
public:

    explicit ThreadPool(std::size_t threads = std::thread::hardware_concurrency()) 
            :lock_(), cond_(), tasks_(), threads_() {
        this->threads_.reserve(threads);
        for (std::size_t i = 0; i != threads; ++i) {
            this->threads_.emplace_back(&Self::ThreadEntry, this);
        }
    }
    ThreadPool(const Self &other) = delete;
    ThreadPool(Self &&other) noexcept = delete;
    Self &operator=(const Self &other) = delete;
    Self &operator=(Self &&other) noexcept = delete;
    ~ThreadPool() noexcept {
        this->Stop();
    }

    template<typename _Fn, typename ..._Args,
        typename _Check = decltype(
                std::declval<std::function<void()>&>() =
                    std::bind(std::declval<_Fn>(), std::declval<_Args>()...)
            )>
    inline void SubmitTask(_Fn &&fn, _Args &&...args) {
        std::unique_lock<std::mutex> lock(this->lock_);
        this->tasks_.emplace_back(std::bind(std::forward<_Fn>(fn), std::forward<_Args>(args)...));
        this->cond_.notify_one();
    }
    void Stop() noexcept {
        if (this->running_) {
            this->running_ = false;
            {
                std::unique_lock<std::mutex> lock(this->lock_);
                this->cond_.notify_all();
            }
            //wait all threads
            for (auto begin = this->threads_.begin(), end = this->threads_.end(); 
                    begin != end; ++begin) {
                begin->join();
            }
        }
    }
};

} // namespace cjs
#endif