/**
 * @file ThreadPool.cpp
 * @brief Thread pool implementation
 */

#include "ThreadPool.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace OrderBook {

    ThreadPool::ThreadPool(size_t num_threads) 
        : stop_(false)
        , tasks_completed_(0)
        , tasks_submitted_(0)
    {
        if (num_threads == 0) {
            num_threads = std::thread::hardware_concurrency();
            if (num_threads == 0) {
                num_threads = 4; // Fallback
            }
        }
        
        workers_.reserve(num_threads);
        
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back(&ThreadPool::worker, this);
        }
    }

    ThreadPool::~ThreadPool() {
        stop();
    }

    size_t ThreadPool::getPendingTaskCount() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }

    void ThreadPool::stop() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_.store(true);
        }
        
        condition_.notify_all();
        
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void ThreadPool::waitForAll() {
        while (getPendingTaskCount() > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    std::string ThreadPool::getStats() const {
        std::ostringstream oss;
        oss << "ThreadPool Statistics:\n";
        oss << "  Worker Threads: " << workers_.size() << "\n";
        oss << "  Tasks Submitted: " << tasks_submitted_.load() << "\n";
        oss << "  Tasks Completed: " << tasks_completed_.load() << "\n";
        oss << "  Pending Tasks: " << getPendingTaskCount() << "\n";
        oss << "  Stopped: " << (stop_.load() ? "Yes" : "No") << "\n";
        return oss.str();
    }

    void ThreadPool::worker() {
        while (true) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                condition_.wait(lock, [this] { return stop_.load() || !tasks_.empty(); });
                
                if (stop_.load() && tasks_.empty()) {
                    return;
                }
                
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            
            try {
                task();
                tasks_completed_.fetch_add(1);
            } catch (const std::exception& e) {
                // Log error but continue processing
                std::cerr << "ThreadPool task error: " << e.what() << std::endl;
            }
        }
    }

} // namespace OrderBook
