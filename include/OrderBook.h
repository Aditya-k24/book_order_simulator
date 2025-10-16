/**
 * @file OrderBook.h
 * @brief OrderBook class definition for maintaining bid/ask price levels
 * @author Trading Systems Engineer
 * @date 2024
 */

#pragma once

#include "Order.h"
#include <map>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <memory>

namespace OrderBook {

    /**
     * @struct PriceLevel
     * @brief Represents a price level with total quantity and order list
     */
    struct PriceLevel {
        uint64_t price;                    ///< Price level
        uint64_t total_quantity;           ///< Total quantity at this price
        std::vector<std::shared_ptr<Order>> orders; ///< Orders at this price level
        
        PriceLevel() : price(0), total_quantity(0) {}
        PriceLevel(uint64_t p) : price(p), total_quantity(0) {}
        
        /**
         * @brief Add order to this price level
         * @param order Shared pointer to order
         */
        void addOrder(std::shared_ptr<Order> order) {
            orders.push_back(order);
            total_quantity += order->getRemainingQuantity();
        }
        
        /**
         * @brief Remove order from this price level
         * @param order_id Order ID to remove
         * @return true if order was found and removed
         */
        bool removeOrder(Order::OrderID order_id) {
            for (auto it = orders.begin(); it != orders.end(); ++it) {
                if ((*it)->getId() == order_id) {
                    total_quantity -= (*it)->getRemainingQuantity();
                    orders.erase(it);
                    return true;
                }
            }
            return false;
        }
        
        /**
         * @brief Update quantity after partial fill
         * @param order_id Order ID to update
         * @param old_qty Previous quantity
         * @param new_qty New quantity
         */
        void updateQuantity(Order::OrderID /* order_id */, uint64_t old_qty, uint64_t new_qty) {
            total_quantity = total_quantity - old_qty + new_qty;
        }
        
        /**
         * @brief Check if price level is empty
         * @return true if no orders remain
         */
        bool isEmpty() const {
            return orders.empty();
        }
        
        /**
         * @brief Clear all orders from this price level
         */
        void clear() {
            orders.clear();
            total_quantity = 0;
        }
    };

    /**
     * @class OrderBook
     * @brief High-performance order book implementation
     * 
     * Uses std::map for O(log n) insertion/lookup and maintains
     * separate bid and ask books with price-time priority.
     * Thread-safe with minimal locking for high-frequency trading.
     */
    class OrderBook {
    public:
        using PriceLevelMap = std::map<uint64_t, PriceLevel>;
        using OrderMap = std::unordered_map<Order::OrderID, std::shared_ptr<Order>>;

        /**
         * @brief Constructor
         * @param symbol Trading symbol (e.g., "AAPL")
         */
        explicit OrderBook(const std::string& symbol = "DEFAULT");

        /**
         * @brief Destructor
         */
        ~OrderBook() = default;

        // Non-copyable and non-movable (due to mutex)
        OrderBook(const OrderBook&) = delete;
        OrderBook& operator=(const OrderBook&) = delete;
        OrderBook(OrderBook&&) = delete;
        OrderBook& operator=(OrderBook&&) = delete;

        /**
         * @brief Add order to the order book
         * @param order Shared pointer to order
         * @return true if order was successfully added
         */
        bool addOrder(std::shared_ptr<Order> order);

        /**
         * @brief Cancel order by ID
         * @param order_id Order ID to cancel
         * @return true if order was found and cancelled
         */
        bool cancelOrder(Order::OrderID order_id);

        /**
         * @brief Get best bid price
         * @return Best bid price, 0 if no bids
         */
        uint64_t getBestBid() const;

        /**
         * @brief Get best ask price
         * @return Best ask price, 0 if no asks
         */
        uint64_t getBestAsk() const;

        /**
         * @brief Get spread (ask - bid)
         * @return Spread in basis points, 0 if no spread
         */
        uint64_t getSpread() const;

        /**
         * @brief Get total quantity at best bid
         * @return Total quantity at best bid
         */
        uint64_t getBestBidQuantity() const;

        /**
         * @brief Get total quantity at best ask
         * @return Total quantity at best ask
         */
        uint64_t getBestAskQuantity() const;

        /**
         * @brief Get order by ID
         * @param order_id Order ID
         * @return Shared pointer to order, nullptr if not found
         */
        std::shared_ptr<Order> getOrder(Order::OrderID order_id) const;

        /**
         * @brief Get all orders at a specific price level
         * @param price Price level
         * @param side Order side
         * @return Vector of orders at the price level
         */
        std::vector<std::shared_ptr<Order>> getOrdersAtPrice(uint64_t price, OrderSide side) const;

        /**
         * @brief Get market depth (top N levels)
         * @param levels Number of levels to return
         * @return Pair of bid and ask price levels
         */
        std::pair<std::vector<std::pair<uint64_t, uint64_t>>, 
                  std::vector<std::pair<uint64_t, uint64_t>>> 
        getMarketDepth(size_t levels = 10) const;

        /**
         * @brief Get total number of orders
         * @return Total order count
         */
        size_t getOrderCount() const;

        /**
         * @brief Get trading symbol
         * @return Symbol string
         */
        const std::string& getSymbol() const { return symbol_; }

        /**
         * @brief Check if order book is empty
         * @return true if no orders in book
         */
        bool isEmpty() const;

        /**
         * @brief Clear all orders from the book
         */
        void clear();

        /**
         * @brief Get string representation of order book
         * @param levels Number of levels to display
         * @return Formatted string
         */
        std::string toString(size_t levels = 5) const;

        /**
         * @brief Get best bid/ask for matching
         * @return Pair of (best_bid_price, best_ask_price)
         */
        std::pair<uint64_t, uint64_t> getBestPrices() const;

        /**
         * @brief Get orders for matching at best prices
         * @param side Side to get orders for
         * @return Vector of orders at best price
         */
        std::vector<std::shared_ptr<Order>> getOrdersForMatching(OrderSide side) const;

        /**
         * @brief Update order quantity after partial fill
         * @param order_id Order ID
         * @param old_qty Previous quantity
         * @param new_qty New quantity
         */
        void updateOrderQuantity(Order::OrderID order_id, uint64_t old_qty, uint64_t new_qty);

    private:
        std::string symbol_;                    ///< Trading symbol
        PriceLevelMap bids_;                    ///< Bid price levels (price -> PriceLevel)
        PriceLevelMap asks_;                    ///< Ask price levels (price -> PriceLevel)
        OrderMap orders_;                       ///< All orders by ID for O(1) lookup
        mutable std::mutex book_mutex_;  ///< Mutex for thread safety
        
        /**
         * @brief Get price level map for given side
         * @param side Order side
         * @return Reference to price level map
         */
        PriceLevelMap& getPriceLevelMap(OrderSide side);
        
        /**
         * @brief Get const price level map for given side
         * @param side Order side
         * @return Const reference to price level map
         */
        const PriceLevelMap& getPriceLevelMap(OrderSide side) const;
        
        /**
         * @brief Remove empty price levels
         * @param side Order side
         * @param price Price level to check
         */
        void removeEmptyPriceLevel(OrderSide side, uint64_t price);
    };

} // namespace OrderBook
