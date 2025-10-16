/**
 * @file main.cpp
 * @brief Low-Latency Order Book Simulator - Main Entry Point
 * @author Trading Systems Engineer
 * @date 2024
 * 
 * Demonstrates high-frequency trading infrastructure capabilities
 * with microsecond-level latency measurement and concurrent processing.
 */

#include "MatchingEngine.h"
#include "ThreadPool.h"
#include "PerformanceMonitor.h"
#include "Order.h"
#include <iostream>
#include <random>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <string>
#include <sstream>
#include <iomanip>

using namespace OrderBook;

/**
 * @brief Configuration parameters for the simulation
 */
struct SimulationConfig {
    size_t num_orders = 100000;           ///< Total number of orders to process
    size_t num_threads = 4;               ///< Number of worker threads
    uint64_t base_price = 10000;          ///< Base price in basis points
    uint64_t price_range = 1000;          ///< Price range (+/- from base)
    uint64_t min_quantity = 1;            ///< Minimum order quantity
    uint64_t max_quantity = 1000;         ///< Maximum order quantity
    double fill_ratio = 0.7;              ///< Expected fill ratio (0.0 to 1.0)
    bool enable_csv_logging = true;       ///< Enable CSV trade logging
    bool enable_performance_monitoring = true; ///< Enable performance monitoring
    std::string symbol = "AAPL";          ///< Trading symbol
    size_t batch_size = 100;              ///< Batch size for processing
};

/**
 * @brief Order generator for simulation
 */
class OrderGenerator {
public:
    OrderGenerator(const SimulationConfig& config)
        : config_(config)
        , rng_(std::random_device{}())
        , price_dist_(config.base_price - config.price_range, 
                     config.base_price + config.price_range)
        , quantity_dist_(config.min_quantity, config.max_quantity)
        , side_dist_(0, 1)
        , order_id_counter_(1)
    {
    }

    /**
     * @brief Generate a batch of random orders
     * @param batch_size Number of orders to generate
     * @return Vector of generated orders
     */
    std::vector<std::shared_ptr<Order>> generateBatch(size_t batch_size) {
        std::vector<std::shared_ptr<Order>> orders;
        orders.reserve(batch_size);

        for (size_t i = 0; i < batch_size; ++i) {
            auto order = generateOrder();
            orders.push_back(order);
        }

        return orders;
    }

    /**
     * @brief Generate a single random order
     * @return Generated order
     */
    std::shared_ptr<Order> generateOrder() {
        uint64_t price = price_dist_(rng_);
        uint64_t quantity = quantity_dist_(rng_);
        OrderSide side = (side_dist_(rng_) == 0) ? OrderSide::BUY : OrderSide::SELL;
        
        auto now = std::chrono::high_resolution_clock::now();
        auto order = std::make_shared<Order>(
            order_id_counter_++, side, price, quantity, now
        );
        
        return order;
    }

    /**
     * @brief Generate aggressive orders that will likely match
     * @param num_orders Number of aggressive orders to generate
     * @return Vector of aggressive orders
     */
    std::vector<std::shared_ptr<Order>> generateAggressiveOrders(size_t num_orders) {
        std::vector<std::shared_ptr<Order>> orders;
        orders.reserve(num_orders);

        // Generate some initial orders to create a book
        auto initial_orders = generateBatch(num_orders / 2);
        
        // Generate aggressive orders that cross the spread
        for (size_t i = 0; i < num_orders / 2; ++i) {
            uint64_t quantity = quantity_dist_(rng_);
            OrderSide side = (side_dist_(rng_) == 0) ? OrderSide::BUY : OrderSide::SELL;
            
            // Generate aggressive prices that will cross the spread
            uint64_t aggressive_price;
            if (side == OrderSide::BUY) {
                // Buy orders at higher prices (more aggressive)
                aggressive_price = config_.base_price + config_.price_range + 
                                 quantity_dist_(rng_) % 500;
            } else {
                // Sell orders at lower prices (more aggressive)
                aggressive_price = config_.base_price - config_.price_range - 
                                 quantity_dist_(rng_) % 500;
            }
            
            auto now = std::chrono::high_resolution_clock::now();
            auto order = std::make_shared<Order>(
                order_id_counter_++, side, aggressive_price, quantity, now
            );
            
            orders.push_back(order);
        }

        // Add initial orders
        orders.insert(orders.end(), initial_orders.begin(), initial_orders.end());
        
        return orders;
    }

private:
    SimulationConfig config_;
    std::mt19937 rng_;
    std::uniform_int_distribution<uint64_t> price_dist_;
    std::uniform_int_distribution<uint64_t> quantity_dist_;
    std::uniform_int_distribution<int> side_dist_;
    std::atomic<uint64_t> order_id_counter_;
};

/**
 * @brief Benchmark different data structure implementations
 */
void runBenchmark() {
    std::cout << "\n=== Running Benchmark Tests ===" << std::endl;
    
    SimulationConfig config;
    config.num_orders = 50000;
    config.num_threads = 4;
    
    PerformanceMonitor monitor(true);
    MatchingEngine engine(config.symbol);
    engine.setCSVLogging(true, "benchmark_trades.csv");
    
    OrderGenerator generator(config);
    
    std::cout << "Generating " << config.num_orders << " orders..." << std::endl;
    auto orders = generator.generateBatch(config.num_orders);
    
    std::cout << "Processing orders..." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    size_t processed = 0;
    for (const auto& order : orders) {
        TIME_OPERATION(monitor, "order_submission", order->getId());
        if (engine.submitOrder(order)) {
            processed++;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time).count();
    
    std::cout << "\nBenchmark Results:" << std::endl;
    std::cout << "Orders Processed: " << processed << std::endl;
    std::cout << "Total Time: " << total_time << " microseconds" << std::endl;
    std::cout << "Throughput: " << (processed * 1000000.0 / total_time) 
              << " orders/second" << std::endl;
    
    monitor.printStats();
    std::cout << engine.getMarketStats() << std::endl;
}

/**
 * @brief Run multi-threaded simulation
 */
void runMultiThreadedSimulation(const SimulationConfig& config) {
    std::cout << "\n=== Multi-Threaded Simulation ===" << std::endl;
    std::cout << "Orders: " << config.num_orders << std::endl;
    std::cout << "Threads: " << config.num_threads << std::endl;
    std::cout << "Symbol: " << config.symbol << std::endl;
    
    // Initialize components
    PerformanceMonitor monitor(config.enable_performance_monitoring);
    MatchingEngine engine(config.symbol);
    ThreadPool thread_pool(config.num_threads);
    
    if (config.enable_csv_logging) {
        engine.setCSVLogging(true, "simulation_trades.csv");
    }
    
    OrderGenerator generator(config);
    
    // Generate orders
    std::cout << "Generating orders..." << std::endl;
    auto orders = generator.generateBatch(config.num_orders);
    
    // Process orders in batches using thread pool
    std::cout << "Processing orders with thread pool..." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::atomic<size_t> processed_count(0);
    std::vector<std::future<size_t>> futures;
    
    // Submit batches to thread pool
    for (size_t i = 0; i < orders.size(); i += config.batch_size) {
        size_t batch_end = std::min(i + config.batch_size, orders.size());
        std::vector<std::shared_ptr<Order>> batch(
            orders.begin() + i, orders.begin() + batch_end
        );
        
        auto future = thread_pool.submit([&engine, &monitor, &processed_count, batch]() {
            size_t batch_processed = 0;
            for (const auto& order : batch) {
                TIME_OPERATION(monitor, "order_submission", order->getId());
                if (engine.submitOrder(order)) {
                    batch_processed++;
                    processed_count.fetch_add(1);
                }
            }
            return batch_processed;
        });
        
        futures.push_back(std::move(future));
    }
    
    // Wait for all batches to complete
    size_t total_processed = 0;
    for (auto& future : futures) {
        total_processed += future.get();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time).count();
    
    // Print results
    std::cout << "\nSimulation Results:" << std::endl;
    std::cout << "Orders Processed: " << total_processed << std::endl;
    std::cout << "Total Time: " << total_time << " microseconds" << std::endl;
    std::cout << "Throughput: " << (total_processed * 1000000.0 / total_time) 
              << " orders/second" << std::endl;
    
    if (config.enable_performance_monitoring) {
        monitor.printStats();
    }
    
    std::cout << engine.getMarketStats() << std::endl;
    std::cout << thread_pool.getStats() << std::endl;
}

/**
 * @brief Run aggressive order simulation to test matching
 */
void runAggressiveSimulation(const SimulationConfig& config) {
    std::cout << "\n=== Aggressive Order Simulation ===" << std::endl;
    
    PerformanceMonitor monitor(true);
    MatchingEngine engine(config.symbol);
    engine.setCSVLogging(true, "aggressive_trades.csv");
    
    OrderGenerator generator(config);
    
    std::cout << "Generating aggressive orders for maximum matching..." << std::endl;
    auto orders = generator.generateAggressiveOrders(config.num_orders);
    
    std::cout << "Processing " << orders.size() << " orders..." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    size_t processed = 0;
    for (const auto& order : orders) {
        TIME_OPERATION(monitor, "order_submission", order->getId());
        if (engine.submitOrder(order)) {
            processed++;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time).count();
    
    std::cout << "\nAggressive Simulation Results:" << std::endl;
    std::cout << "Orders Processed: " << processed << std::endl;
    std::cout << "Trades Executed: " << engine.getTradeCount() << std::endl;
    std::cout << "Total Volume: " << engine.getTotalVolume() << std::endl;
    std::cout << "Fill Rate: " << (engine.getTradeCount() * 2.0 / processed * 100) << "%" << std::endl;
    std::cout << "Total Time: " << total_time << " microseconds" << std::endl;
    std::cout << "Throughput: " << (processed * 1000000.0 / total_time) 
              << " orders/second" << std::endl;
    
    monitor.printStats();
    std::cout << engine.getMarketStats() << std::endl;
    
    // Show final order book state
    std::cout << "\nFinal Order Book State:" << std::endl;
    std::cout << engine.getOrderBookSnapshot(10) << std::endl;
}

/**
 * @brief Print usage information
 */
void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --benchmark          Run benchmark tests" << std::endl;
    std::cout << "  --aggressive         Run aggressive order simulation" << std::endl;
    std::cout << "  --orders N           Number of orders (default: 100000)" << std::endl;
    std::cout << "  --threads N          Number of threads (default: 4)" << std::endl;
    std::cout << "  --symbol SYMBOL      Trading symbol (default: AAPL)" << std::endl;
    std::cout << "  --no-csv             Disable CSV logging" << std::endl;
    std::cout << "  --no-perf            Disable performance monitoring" << std::endl;
    std::cout << "  --help               Show this help message" << std::endl;
}

/**
 * @brief Parse command line arguments
 */
SimulationConfig parseArguments(int argc, char* argv[]) {
    SimulationConfig config;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            printUsage(argv[0]);
            exit(0);
        } else if (arg == "--benchmark") {
            runBenchmark();
            exit(0);
        } else if (arg == "--aggressive") {
            runAggressiveSimulation(config);
            exit(0);
        } else if (arg == "--orders" && i + 1 < argc) {
            config.num_orders = std::stoul(argv[++i]);
        } else if (arg == "--threads" && i + 1 < argc) {
            config.num_threads = std::stoul(argv[++i]);
        } else if (arg == "--symbol" && i + 1 < argc) {
            config.symbol = argv[++i];
        } else if (arg == "--no-csv") {
            config.enable_csv_logging = false;
        } else if (arg == "--no-perf") {
            config.enable_performance_monitoring = false;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            exit(1);
        }
    }
    
    return config;
}

/**
 * @brief Main entry point
 */
int main(int argc, char* argv[]) {
    std::cout << "==========================================" << std::endl;
    std::cout << "  Low-Latency Order Book Simulator" << std::endl;
    std::cout << "  High-Frequency Trading Infrastructure" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    try {
        // Parse command line arguments
        SimulationConfig config = parseArguments(argc, argv);
        
        // Run the main simulation
        runMultiThreadedSimulation(config);
        
        std::cout << "\nSimulation completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
