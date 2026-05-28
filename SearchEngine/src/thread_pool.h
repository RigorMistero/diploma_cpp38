#pragma once

#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <memory>     
#include <stdexcept>   

class ThreadPool 
{
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    // add task to queue , future for result
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result_t<F, Args...>>;  
    
    void wait_all();

    // num of threads 
    size_t size() const;

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::condition_variable done_cv_;

    bool stop_ = false;
    size_t active_tasks_ = 0;
    std::mutex done_mutex_;
};

template<typename F, typename... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result_t<F, Args...>>  
{
    using return_type = typename std::invoke_result_t<F, Args...>;

    // task to packaged_task
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> result = task->get_future();

    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_) {
            throw std::runtime_error("ThreadPool is stopped");
        }
        ++active_tasks_;
        tasks_.emplace([task]() { (*task)(); });  
    }
    cv_.notify_one();

    return result;
}

