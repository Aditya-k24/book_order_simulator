/**
 * @file Trade.h
 * @brief Trade execution record definition
 * @author Trading Systems Engineer
 * @date 2024
 */

#pragma once

#include "Order.h"
#include <chrono>

namespace OrderBook {

    /**
     * @struct Trade
     * @brief Represents a completed trade between two orders
     */
    struct Trade {
        Order::OrderID buy_order_id;     ///< ID of the buy order
        Order::OrderID sell_order_id;    ///< ID of the sell order
        uint64_t price;                  ///< Execution price
        uint64_t quantity;               ///< Trade quantity
        std::chrono::high_resolution_clock::time_point timestamp; ///< Execution timestamp
        
        /**
         * @brief Constructor
         * @param buy_id Buy order ID
         * @param sell_id Sell order ID
         * @param p Execution price
         * @param q Trade quantity
         * @param ts Execution timestamp
         */
        Trade(Order::OrderID buy_id, Order::OrderID sell_id, uint64_t p, uint64_t q, 
              std::chrono::high_resolution_clock::time_point ts)
            : buy_order_id(buy_id)
            , sell_order_id(sell_id)
            , price(p)
            , quantity(q)
            , timestamp(ts)
        {
        }
        
        /**
         * @brief Get trade as CSV string
         * @return Formatted CSV line
         */
        std::string toCSV() const;
        
        /**
         * @brief Get trade as string
         * @return Formatted string representation
         */
        std::string toString() const;
    };

} // namespace OrderBook
