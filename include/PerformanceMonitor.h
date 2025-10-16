/**
 * @file PerformanceMonitor.h
 * @brief Performance monitoring and latency measurement utilities
 * @author Trading Systems Engineer
 * @date 2024
 */

#pragma once

#include <chrono>
#include <vector>
#include <atomic>
#include <mutex>
#include <string>
#include <fstream>
#include <memory>

namespace OrderBook {

    /**
     * @class LatencyMeasurement
     * @brief Single latency measurement record
     */
    struct LatencyMeasurement {
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point end_time;
        uint64_t order_id;
        std::string operation_type;
        
        /**
         * @brief Get latency in nanoseconds
         * @return Latency in nanoseconds
         */
        uint64_t getLatencyNanoseconds() const {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(
                end_time - start_time).count();
        }
        
        /**
         * @brief Get latency in microseconds
         * @return Latency in microseconds
         */
        double getLatencyMicroseconds() const {
            return getLatencyNanoseconds() / 1000.0;
        }
    };

    /**
     * @class PerformanceStats
     * @brief Aggregated performance statistics
     */
    struct PerformanceStats {
        uint64_t total_operations;
        uint64_t min_latency_ns;
        uint64_t max_latency_ns;
        double mean_latency_ns;
        double median_latency_ns;
        double p95_latency_ns;
        double p99_latency_ns;
        double std_deviation_ns;
        uint64_t total_duration_ns;
        double throughput_ops_per_sec;
        
        PerformanceStats() 
            : total_operations(0)
            , min_latency_ns(UINT64_MAX)
            , max_latency_ns(0)
            , mean_latency_ns(0.0)
            , median_latency_ns(0.0)
            , p95_latency_ns(0.0)
            , p99_latency_ns(0.0)
            , std_deviation_ns(0.0)
            , total_duration_ns(0)
            , throughput_ops_per_sec(0.0)
        {}
    };

    /**
     * @class PerformanceMonitor
     * @brief High-performance monitoring system for latency measurement
     * 
     * Thread-safe performance monitor that tracks latency distributions,
     * throughput, and provides detailed statistics for trading systems.
     */
    class PerformanceMonitor {
    public:
        /**
         * @brief Constructor
         * @param enable_detailed_logging Enable detailed per-operation logging
         */
        explicit PerformanceMonitor(bool enable_detailed_logging = false);

        /**
         * @brief Destructor
         */
        ~PerformanceMonitor();

        // Non-copyable and non-movable (due to mutex)
        PerformanceMonitor(const PerformanceMonitor&) = delete;
        PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;
        PerformanceMonitor(PerformanceMonitor&&) = delete;
        PerformanceMonitor& operator=(PerformanceMonitor&&) = delete;

        /**
         * @brief Start timing an operation
         * @param operation_type Type of operation being measured
         * @param order_id Associated order ID
         * @return Start timestamp
         */
        std::chrono::high_resolution_clock::time_point startTiming(
            const std::string& operation_type, 
            uint64_t order_id = 0);

        /**
         * @brief End timing an operation
         * @param start_time Start timestamp from startTiming()
         * @param operation_type Type of operation
         * @param order_id Associated order ID
         */
        void endTiming(
            std::chrono::high_resolution_clock::time_point start_time,
            const std::string& operation_type,
            uint64_t order_id = 0);

        /**
         * @brief Record a completed operation with latency
         * @param latency_ns Latency in nanoseconds
         * @param operation_type Type of operation
         * @param order_id Associated order ID
         */
        void recordOperation(
            uint64_t latency_ns,
            const std::string& operation_type,
            uint64_t order_id = 0);

        /**
         * @brief Get statistics for a specific operation type
         * @param operation_type Operation type to get stats for
         * @return Performance statistics
         */
        PerformanceStats getStats(const std::string& operation_type) const;

        /**
         * @brief Get overall statistics (all operations)
         * @return Overall performance statistics
         */
        PerformanceStats getOverallStats() const;

        /**
         * @brief Get all operation types being tracked
         * @return Vector of operation type strings
         */
        std::vector<std::string> getOperationTypes() const;

        /**
         * @brief Clear all measurements
         */
        void clear();

        /**
         * @brief Export measurements to CSV file
         * @param filename Output filename
         * @return true if successful
         */
        bool exportToCSV(const std::string& filename) const;

        /**
         * @brief Print detailed statistics to console
         * @param operation_type Specific operation type (empty for all)
         */
        void printStats(const std::string& operation_type = "") const;

        /**
         * @brief Get throughput in operations per second
         * @param operation_type Specific operation type (empty for all)
         * @return Throughput in ops/sec
         */
        double getThroughput(const std::string& operation_type = "") const;

        /**
         * @brief Enable/disable detailed logging
         * @param enable true to enable
         */
        void setDetailedLogging(bool enable);

        /**
         * @brief Get total number of measurements
         * @return Measurement count
         */
        uint64_t getMeasurementCount() const;

    private:
        struct OperationData {
            std::vector<uint64_t> latencies;
            std::atomic<uint64_t> total_count;
            std::atomic<uint64_t> total_latency;
            mutable std::mutex mutex;
            
            OperationData() : total_count(0), total_latency(0) {}
        };

        bool detailed_logging_enabled_;
        std::unordered_map<std::string, OperationData> operation_data_;
        std::vector<LatencyMeasurement> detailed_measurements_;
        mutable std::mutex global_mutex_;
        
        /**
         * @brief Calculate statistics from latency vector
         * @param latencies Vector of latency measurements
         * @return Calculated statistics
         */
        PerformanceStats calculateStats(const std::vector<uint64_t>& latencies) const;
        
        /**
         * @brief Get percentile value from sorted vector
         * @param sorted_data Sorted latency data
         * @param percentile Percentile (0.0 to 1.0)
         * @return Percentile value
         */
        double getPercentile(const std::vector<uint64_t>& sorted_data, double percentile) const;
    };

    /**
     * @class ScopedTimer
     * @brief RAII-style timer for automatic latency measurement
     */
    class ScopedTimer {
    public:
        ScopedTimer(PerformanceMonitor& monitor, 
                   const std::string& operation_type,
                   uint64_t order_id = 0);
        ~ScopedTimer();

    private:
        PerformanceMonitor& monitor_;
        std::string operation_type_;
        uint64_t order_id_;
        std::chrono::high_resolution_clock::time_point start_time_;
    };

    // Macro for easy timing
    #define TIME_OPERATION(monitor, op_type, order_id) \
        ScopedTimer _timer(monitor, op_type, order_id)

} // namespace OrderBook
