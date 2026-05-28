#include "thread_pool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t num_threads) : stop_(false), active_tasks_(0) 
{
    if (num_threads == 0) 
    {
        num_threads = std::thread::hardware_concurrency();
    }

    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) 
    {
        workers_.emplace_back([this]() 
            {
            while (true) 
            {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    cv_.wait(lock, [this]() 
                        {
                        return stop_ || !tasks_.empty();
                    });

                    if (stop_ && tasks_.empty()) 
                    {
                        return;
                    }

                    task = std::move(tasks_.front());
                    tasks_.pop();
                }

                // completing task
                task();

                {
                    std::lock_guard<std::mutex> lock(done_mutex_);
                    --active_tasks_;
                }
                done_cv_.notify_all();
            }
        });
    }
}

ThreadPool::~ThreadPool() 
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    cv_.notify_all();

    for (auto& worker : workers_) 
    {
        if (worker.joinable()) 
        {
            worker.join();
        }
    }
}

void ThreadPool::wait_all() 
{
    std::unique_lock<std::mutex> lock(done_mutex_);
    done_cv_.wait(lock, [this]() 
        {
        std::lock_guard<std::mutex> qlock(queue_mutex_);
        return active_tasks_ == 0 && tasks_.empty();
    });
}

size_t ThreadPool::size() const 
{
    return workers_.size();
}


