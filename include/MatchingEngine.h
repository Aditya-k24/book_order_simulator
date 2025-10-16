/**
 * @file MatchingEngine.h
 * @brief Matching engine for order book simulation
 * @author Trading Systems Engineer
 * @date 2024
 */

#pragma once

#include "OrderBook.h"
#include "Trade.h"
#include <memory>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <fstream>

namespace OrderBook {

    /**
     * @class MatchingEngine
     * @brief High-performance matching engine with price-time priority
     * 
     * Implements a continuous double auction matching algorithm with
     * price-time priority. Optimized for low-latency order processing
     * and trade execution logging.
     */
    class MatchingEngine {
    public:
        using TradeCallback = std::function<void(const Trade&)>;
        using OrderCallback = std::function<void(std::shared_ptr<Order>)>;

        /**
         * @brief Constructor
         * @param symbol Trading symbol
         */
        explicit MatchingEngine(const std::string& symbol = "DEFAULT");

        /**
         * @brief Destructor
         */
        ~MatchingEngine();

        // Non-copyable and non-movable (due to OrderBook)
        MatchingEngine(const MatchingEngine&) = delete;
        MatchingEngine& operator=(const MatchingEngine&) = delete;
        MatchingEngine(MatchingEngine&&) = delete;
        MatchingEngine& operator=(MatchingEngine&&) = delete;

        /**
         * @brief Submit a new order for matching
         * @param order Shared pointer to order
         * @return true if order was successfully submitted
         */
        bool submitOrder(std::shared_ptr<Order> order);

        /**
         * @brief Cancel an existing order
         * @param order_id Order ID to cancel
         * @return true if order was found and cancelled
         */
        bool cancelOrder(Order::OrderID order_id);

        /**
         * @brief Get reference to the order book
         * @return Const reference to order book
         */
        const OrderBook& getOrderBook() const { return order_book_; }

        /**
         * @brief Get total number of trades executed
         * @return Trade count
         */
        uint64_t getTradeCount() const { return trade_count_.load(); }

        /**
         * @brief Get total volume traded
         * @return Total volume
         */
        uint64_t getTotalVolume() const { return total_volume_.load(); }

        /**
         * @brief Get total value traded
         * @return Total value (volume * average price)
         */
        uint64_t getTotalValue() const { return total_value_.load(); }

        /**
         * @brief Get all executed trades
         * @return Const reference to trades vector
         */
        const std::vector<Trade>& getTrades() const { return trades_; }

        /**
         * @brief Set trade callback function
         * @param callback Function to call on trade execution
         */
        void setTradeCallback(TradeCallback callback) { trade_callback_ = callback; }

        /**
         * @brief Set order callback function
         * @param callback Function to call on order events
         */
        void setOrderCallback(OrderCallback callback) { order_callback_ = callback; }

        /**
         * @brief Enable/disable CSV trade logging
         * @param enable true to enable logging
         * @param filename CSV filename (default: trades.csv)
         */
        void setCSVLogging(bool enable, const std::string& filename = "trades.csv");

        /**
         * @brief Get trading symbol
         * @return Symbol string
         */
        const std::string& getSymbol() const { return symbol_; }

        /**
         * @brief Get order book snapshot as string
         * @param levels Number of levels to display
         * @return Formatted order book string
         */
        std::string getOrderBookSnapshot(size_t levels = 5) const;

        /**
         * @brief Get market statistics
         * @return String with market stats
         */
        std::string getMarketStats() const;

        /**
         * @brief Clear all orders and trades
         */
        void clear();

        /**
         * @brief Process a batch of orders
         * @param orders Vector of orders to process
         * @return Number of orders successfully processed
         */
        size_t processBatch(const std::vector<std::shared_ptr<Order>>& orders);

    private:
        std::string symbol_;                          ///< Trading symbol
        OrderBook order_book_;                        ///< Order book instance
        std::vector<Trade> trades_;                   ///< Executed trades
        std::atomic<uint64_t> trade_count_;           ///< Total trade count
        std::atomic<uint64_t> total_volume_;          ///< Total volume traded
        std::atomic<uint64_t> total_value_;           ///< Total value traded
        
        // Callbacks
        TradeCallback trade_callback_;                ///< Trade execution callback
        OrderCallback order_callback_;                ///< Order event callback
        
        // CSV logging
        bool csv_logging_enabled_;                    ///< CSV logging flag
        std::string csv_filename_;                    ///< CSV filename
        std::ofstream csv_file_;                      ///< CSV file stream
        mutable std::mutex csv_mutex_;                ///< CSV file mutex
        
        /**
         * @brief Match incoming order against existing orders
         * @param order Order to match
         * @return Number of trades executed
         */
        size_t matchOrder(std::shared_ptr<Order> order);
        
        /**
         * @brief Execute a trade between two orders
         * @param buy_order Buy order
         * @param sell_order Sell order
         * @param trade_price Trade execution price
         * @param trade_quantity Trade quantity
         * @return Trade record
         */
        Trade executeTrade(std::shared_ptr<Order> buy_order, 
                          std::shared_ptr<Order> sell_order,
                          uint64_t trade_price, 
                          uint64_t trade_quantity);
        
        /**
         * @brief Log trade to CSV file
         * @param trade Trade to log
         */
        void logTradeToCSV(const Trade& trade);
        
        /**
         * @brief Notify trade callback
         * @param trade Trade that was executed
         */
        void notifyTradeCallback(const Trade& trade);
        
        /**
         * @brief Notify order callback
         * @param order Order event
         */
        void notifyOrderCallback(std::shared_ptr<Order> order);
    };

} // namespace OrderBook
