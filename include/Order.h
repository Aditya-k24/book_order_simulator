/**
 * @file Order.h
 * @brief Order class definition for the low-latency order book simulator
 * @author Trading Systems Engineer
 * @date 2024
 */

#pragma once

#include <chrono>
#include <string>
#include <atomic>

namespace OrderBook {

    /**
     * @enum OrderSide
     * @brief Enumeration for buy/sell order sides
     */
    enum class OrderSide {
        BUY,
        SELL
    };

    /**
     * @enum OrderType
     * @brief Enumeration for order types
     */
    enum class OrderType {
        LIMIT,
        MARKET
    };

    /**
     * @class Order
     * @brief Represents a single order in the order book
     * 
     * This class encapsulates all order information including price, quantity,
     * timestamp, and metadata. Optimized for low-latency operations with
     * minimal memory footprint and cache-friendly layout.
     */
    class Order {
    public:
        using TimePoint = std::chrono::high_resolution_clock::time_point;
        using OrderID = uint64_t;

        /**
         * @brief Constructor for creating a new order
         * @param id Unique order identifier
         * @param side Buy or sell side
         * @param price Order price (in basis points for precision)
         * @param quantity Order quantity
         * @param timestamp Order creation timestamp
         */
        Order(OrderID id, OrderSide side, uint64_t price, uint64_t quantity, TimePoint timestamp);

        /**
         * @brief Default constructor (for STL containers)
         */
        Order() = default;

        /**
         * @brief Copy constructor
         */
        Order(const Order& other) = default;

        /**
         * @brief Assignment operator
         */
        Order& operator=(const Order& other) = default;

        // Getters
        OrderID getId() const noexcept { return id_; }
        OrderSide getSide() const noexcept { return side_; }
        uint64_t getPrice() const noexcept { return price_; }
        uint64_t getQuantity() const noexcept { return quantity_; }
        uint64_t getRemainingQuantity() const noexcept { return remaining_quantity_; }
        TimePoint getTimestamp() const noexcept { return timestamp_; }
        OrderType getType() const noexcept { return type_; }

        // Setters
        void setRemainingQuantity(uint64_t qty) noexcept { remaining_quantity_ = qty; }
        void setType(OrderType type) noexcept { type_ = type; }

        /**
         * @brief Check if order is completely filled
         * @return true if remaining quantity is zero
         */
        bool isFilled() const noexcept { return remaining_quantity_ == 0; }

        /**
         * @brief Check if order is partially filled
         * @return true if remaining quantity is less than original quantity
         */
        bool isPartiallyFilled() const noexcept { 
            return remaining_quantity_ < quantity_ && remaining_quantity_ > 0; 
        }

        /**
         * @brief Reduce remaining quantity by specified amount
         * @param qty Amount to reduce by
         * @return Actual amount reduced (min of qty and remaining_quantity_)
         */
        uint64_t reduceQuantity(uint64_t qty) noexcept {
            uint64_t actual_reduction = std::min(qty, remaining_quantity_);
            remaining_quantity_ -= actual_reduction;
            return actual_reduction;
        }

        /**
         * @brief Get filled quantity
         * @return Original quantity minus remaining quantity
         */
        uint64_t getFilledQuantity() const noexcept { 
            return quantity_ - remaining_quantity_; 
        }

        /**
         * @brief String representation of the order
         * @return Formatted string with order details
         */
        std::string toString() const;

    private:
        OrderID id_;                    ///< Unique order identifier
        OrderSide side_;               ///< Buy or sell side
        uint64_t price_;               ///< Order price (basis points)
        uint64_t quantity_;            ///< Original order quantity
        uint64_t remaining_quantity_;  ///< Remaining quantity to fill
        TimePoint timestamp_;          ///< Order creation timestamp
        OrderType type_;               ///< Order type (LIMIT/MARKET)
    };

    /**
     * @brief Less-than comparison for orders (used in priority queues)
     * Orders are compared by price (for price-time priority)
     */
    struct OrderComparator {
        bool operator()(const Order& lhs, const Order& rhs) const {
            // For buy orders: higher price has higher priority
            // For sell orders: lower price has higher priority
            if (lhs.getSide() == OrderSide::BUY) {
                return lhs.getPrice() < rhs.getPrice();
            } else {
                return lhs.getPrice() > rhs.getPrice();
            }
        }
    };

    /**
     * @brief Equality comparison for orders
     */
    inline bool operator==(const Order& lhs, const Order& rhs) {
        return lhs.getId() == rhs.getId();
    }

} // namespace OrderBook
