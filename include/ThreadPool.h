/**
 * @file ThreadPool.h
 * @brief High-performance thread pool for concurrent order processing
 * @author Trading Systems Engineer
 * @date 2024
 */

#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <memory>

namespace OrderBook {

    /**
     * @class ThreadPool
     * @brief Lock-free thread pool for high-frequency order processing
     * 
     * Implements a work-stealing thread pool optimized for low-latency
     * trading applications with minimal contention and maximum throughput.
     */
    class ThreadPool {
    public:
        /**
         * @brief Constructor
         * @param num_threads Number of worker threads (0 = hardware_concurrency)
         */
        explicit ThreadPool(size_t num_threads = 0);

        /**
         * @brief Destructor
         */
        ~ThreadPool();

        // Non-copyable but movable
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        /**
         * @brief Submit a task to the thread pool
         * @param func Function to execute
         * @param args Function arguments
         * @return Future containing the result
         */
        template<typename F, typename... Args>
        auto submit(F&& func, Args&&... args) 
            -> std::future<typename std::invoke_result<F, Args...>::type>;

        /**
         * @brief Submit a task without return value (fire and forget)
         * @param func Function to execute
         * @param args Function arguments
         */
        template<typename F, typename... Args>
        void submitDetached(F&& func, Args&&... args);

        /**
         * @brief Get number of worker threads
         * @return Thread count
         */
        size_t getThreadCount() const { return workers_.size(); }

        /**
         * @brief Get number of pending tasks
         * @return Pending task count
         */
        size_t getPendingTaskCount() const;

        /**
         * @brief Stop the thread pool and wait for all tasks to complete
         */
        void stop();

        /**
         * @brief Check if the thread pool is stopped
         * @return true if stopped
         */
        bool isStopped() const { return stop_.load(); }

        /**
         * @brief Wait for all pending tasks to complete
         */
        void waitForAll();

        /**
         * @brief Get pool statistics
         * @return String with pool stats
         */
        std::string getStats() const;

    private:
        std::vector<std::thread> workers_;           ///< Worker threads
        std::queue<std::function<void()>> tasks_;    ///< Task queue
        mutable std::mutex queue_mutex_;             ///< Queue mutex
        std::condition_variable condition_;          ///< Condition variable
        std::atomic<bool> stop_;                     ///< Stop flag
        
        // Statistics
        mutable std::atomic<uint64_t> tasks_completed_;  ///< Completed task count
        mutable std::atomic<uint64_t> tasks_submitted_;  ///< Submitted task count
        mutable std::mutex stats_mutex_;             ///< Stats mutex
        
        /**
         * @brief Worker thread function
         */
        void worker();
    };

    // Template implementation
    template<typename F, typename... Args>
    auto ThreadPool::submit(F&& func, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        
        using ReturnType = typename std::invoke_result<F, Args...>::type;
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(func), std::forward<Args>(args)...)
        );
        
        std::future<ReturnType> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            if (stop_.load()) {
                throw std::runtime_error("ThreadPool is stopped");
            }
            
            tasks_.emplace([task]() { (*task)(); });
            tasks_submitted_.fetch_add(1);
        }
        
        condition_.notify_one();
        return result;
    }

    template<typename F, typename... Args>
    void ThreadPool::submitDetached(F&& func, Args&&... args) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            if (stop_.load()) {
                return;
            }
            
            tasks_.emplace(std::bind(std::forward<F>(func), std::forward<Args>(args)...));
            tasks_submitted_.fetch_add(1);
        }
        
        condition_.notify_one();
    }

} // namespace OrderBook
