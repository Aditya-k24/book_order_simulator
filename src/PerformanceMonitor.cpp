/**
 * @file PerformanceMonitor.cpp
 * @brief Performance monitoring implementation
 */

#include "PerformanceMonitor.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace OrderBook {

    PerformanceMonitor::PerformanceMonitor(bool enable_detailed_logging)
        : detailed_logging_enabled_(enable_detailed_logging)
    {
    }

    PerformanceMonitor::~PerformanceMonitor() = default;

    std::chrono::high_resolution_clock::time_point PerformanceMonitor::startTiming(
        const std::string& /* operation_type */, 
        uint64_t /* order_id */) {
        return std::chrono::high_resolution_clock::now();
    }

    void PerformanceMonitor::endTiming(
        std::chrono::high_resolution_clock::time_point start_time,
        const std::string& operation_type,
        uint64_t order_id) {
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time).count();
        
        recordOperation(latency_ns, operation_type, order_id);
    }

    void PerformanceMonitor::recordOperation(
        uint64_t latency_ns,
        const std::string& operation_type,
        uint64_t order_id) {
        
        std::lock_guard<std::mutex> lock(global_mutex_);
        
        auto& data = operation_data_[operation_type];
        {
            std::lock_guard<std::mutex> data_lock(data.mutex);
            data.latencies.push_back(latency_ns);
        }
        
        data.total_count.fetch_add(1);
        data.total_latency.fetch_add(latency_ns);
        
        if (detailed_logging_enabled_) {
            LatencyMeasurement measurement;
            measurement.start_time = std::chrono::high_resolution_clock::now() - 
                                   std::chrono::nanoseconds(latency_ns);
            measurement.end_time = std::chrono::high_resolution_clock::now();
            measurement.order_id = order_id;
            measurement.operation_type = operation_type;
            
            detailed_measurements_.push_back(measurement);
        }
    }

    PerformanceStats PerformanceMonitor::getStats(const std::string& operation_type) const {
        std::lock_guard<std::mutex> lock(global_mutex_);
        
        auto it = operation_data_.find(operation_type);
        if (it == operation_data_.end()) {
            return PerformanceStats();
        }
        
        std::lock_guard<std::mutex> data_lock(it->second.mutex);
        return calculateStats(it->second.latencies);
    }

    PerformanceStats PerformanceMonitor::getOverallStats() const {
        std::lock_guard<std::mutex> lock(global_mutex_);
        
        std::vector<uint64_t> all_latencies;
        // uint64_t total_count = 0; // Not used
        
        for (const auto& [op_type, data] : operation_data_) {
            std::lock_guard<std::mutex> data_lock(data.mutex);
            all_latencies.insert(all_latencies.end(), 
                               data.latencies.begin(), 
                               data.latencies.end());
        }
        
        return calculateStats(all_latencies);
    }

    std::vector<std::string> PerformanceMonitor::getOperationTypes() const {
        std::lock_guard<std::mutex> lock(global_mutex_);
        
        std::vector<std::string> types;
        types.reserve(operation_data_.size());
        
        for (const auto& [op_type, data] : operation_data_) {
            types.push_back(op_type);
        }
        
        return types;
    }

    void PerformanceMonitor::clear() {
        std::lock_guard<std::mutex> lock(global_mutex_);
        
        operation_data_.clear();
        detailed_measurements_.clear();
    }

    bool PerformanceMonitor::exportToCSV(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        file << "operation_type,order_id,latency_ns,latency_us\n";
        
        if (detailed_logging_enabled_) {
            std::lock_guard<std::mutex> lock(global_mutex_);
            
            for (const auto& measurement : detailed_measurements_) {
                file << measurement.operation_type << ","
                     << measurement.order_id << ","
                     << measurement.getLatencyNanoseconds() << ","
                     << std::fixed << std::setprecision(3) 
                     << measurement.getLatencyMicroseconds() << "\n";
            }
        } else {
            std::lock_guard<std::mutex> lock(global_mutex_);
            
            for (const auto& [op_type, data] : operation_data_) {
                std::lock_guard<std::mutex> data_lock(data.mutex);
                
                for (uint64_t latency : data.latencies) {
                    file << op_type << ",0," << latency << ","
                         << std::fixed << std::setprecision(3) 
                         << (latency / 1000.0) << "\n";
                }
            }
        }
        
        return true;
    }

    void PerformanceMonitor::printStats(const std::string& operation_type) const {
        if (operation_type.empty()) {
            // Print overall stats
            auto overall_stats = getOverallStats();
            std::cout << "\n=== Overall Performance Statistics ===" << std::endl;
            std::cout << "Total Operations: " << overall_stats.total_operations << std::endl;
            std::cout << "Min Latency: " << overall_stats.min_latency_ns << " ns" << std::endl;
            std::cout << "Max Latency: " << overall_stats.max_latency_ns << " ns" << std::endl;
            std::cout << "Mean Latency: " << std::fixed << std::setprecision(2) 
                      << overall_stats.mean_latency_ns << " ns" << std::endl;
            std::cout << "Median Latency: " << overall_stats.median_latency_ns << " ns" << std::endl;
            std::cout << "95th Percentile: " << overall_stats.p95_latency_ns << " ns" << std::endl;
            std::cout << "99th Percentile: " << overall_stats.p99_latency_ns << " ns" << std::endl;
            std::cout << "Std Deviation: " << overall_stats.std_deviation_ns << " ns" << std::endl;
            std::cout << "Throughput: " << std::fixed << std::setprecision(2) 
                      << overall_stats.throughput_ops_per_sec << " ops/sec" << std::endl;
            std::cout << "=======================================" << std::endl;
        } else {
            // Print specific operation stats
            auto stats = getStats(operation_type);
            std::cout << "\n=== " << operation_type << " Statistics ===" << std::endl;
            std::cout << "Operations: " << stats.total_operations << std::endl;
            std::cout << "Min Latency: " << stats.min_latency_ns << " ns" << std::endl;
            std::cout << "Max Latency: " << stats.max_latency_ns << " ns" << std::endl;
            std::cout << "Mean Latency: " << std::fixed << std::setprecision(2) 
                      << stats.mean_latency_ns << " ns" << std::endl;
            std::cout << "Median Latency: " << stats.median_latency_ns << " ns" << std::endl;
            std::cout << "95th Percentile: " << stats.p95_latency_ns << " ns" << std::endl;
            std::cout << "99th Percentile: " << stats.p99_latency_ns << " ns" << std::endl;
            std::cout << "Throughput: " << std::fixed << std::setprecision(2) 
                      << stats.throughput_ops_per_sec << " ops/sec" << std::endl;
            std::cout << "================================" << std::endl;
        }
    }

    double PerformanceMonitor::getThroughput(const std::string& operation_type) const {
        auto stats = operation_type.empty() ? getOverallStats() : getStats(operation_type);
        return stats.throughput_ops_per_sec;
    }

    void PerformanceMonitor::setDetailedLogging(bool enable) {
        detailed_logging_enabled_ = enable;
    }

    uint64_t PerformanceMonitor::getMeasurementCount() const {
        std::lock_guard<std::mutex> lock(global_mutex_);
        
        uint64_t total = 0;
        for (const auto& [op_type, data] : operation_data_) {
            total += data.total_count.load();
        }
        return total;
    }

    PerformanceStats PerformanceMonitor::calculateStats(const std::vector<uint64_t>& latencies) const {
        PerformanceStats stats;
        
        if (latencies.empty()) {
            return stats;
        }
        
        stats.total_operations = latencies.size();
        
        auto sorted_latencies = latencies;
        std::sort(sorted_latencies.begin(), sorted_latencies.end());
        
        stats.min_latency_ns = sorted_latencies.front();
        stats.max_latency_ns = sorted_latencies.back();
        
        // Calculate mean
        uint64_t sum = std::accumulate(sorted_latencies.begin(), sorted_latencies.end(), 0ULL);
        stats.mean_latency_ns = static_cast<double>(sum) / latencies.size();
        
        // Calculate median
        if (latencies.size() % 2 == 0) {
            stats.median_latency_ns = (sorted_latencies[latencies.size() / 2 - 1] + 
                                     sorted_latencies[latencies.size() / 2]) / 2.0;
        } else {
            stats.median_latency_ns = sorted_latencies[latencies.size() / 2];
        }
        
        // Calculate percentiles
        stats.p95_latency_ns = getPercentile(sorted_latencies, 0.95);
        stats.p99_latency_ns = getPercentile(sorted_latencies, 0.99);
        
        // Calculate standard deviation
        double variance = 0.0;
        for (uint64_t latency : latencies) {
            double diff = latency - stats.mean_latency_ns;
            variance += diff * diff;
        }
        variance /= latencies.size();
        stats.std_deviation_ns = std::sqrt(variance);
        
        // Estimate throughput (operations per second)
        if (stats.mean_latency_ns > 0) {
            stats.throughput_ops_per_sec = 1e9 / stats.mean_latency_ns;
        }
        
        return stats;
    }

    double PerformanceMonitor::getPercentile(const std::vector<uint64_t>& sorted_data, double percentile) const {
        if (sorted_data.empty()) return 0.0;
        
        double index = percentile * (sorted_data.size() - 1);
        size_t lower_index = static_cast<size_t>(std::floor(index));
        size_t upper_index = static_cast<size_t>(std::ceil(index));
        
        if (lower_index == upper_index) {
            return sorted_data[lower_index];
        }
        
        double weight = index - lower_index;
        return sorted_data[lower_index] * (1 - weight) + sorted_data[upper_index] * weight;
    }

    ScopedTimer::ScopedTimer(PerformanceMonitor& monitor, 
                           const std::string& operation_type,
                           uint64_t order_id)
        : monitor_(monitor)
        , operation_type_(operation_type)
        , order_id_(order_id)
        , start_time_(monitor.startTiming(operation_type, order_id))
    {
    }

    ScopedTimer::~ScopedTimer() {
        monitor_.endTiming(start_time_, operation_type_, order_id_);
    }

} // namespace OrderBook
