/**
 * @file Trade.cpp
 * @brief Trade execution record implementation
 */

#include "Trade.h"
#include <sstream>
#include <iomanip>

namespace OrderBook {

    std::string Trade::toCSV() const {
        std::ostringstream oss;
        auto time_t = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()) % 1000;
        
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        oss << "." << std::setfill('0') << std::setw(3) << ms.count();
        oss << "," << buy_order_id
            << "," << sell_order_id
            << "," << price
            << "," << quantity;
        
        return oss.str();
    }

    std::string Trade::toString() const {
        std::ostringstream oss;
        oss << "Trade{Buy:" << buy_order_id 
            << ", Sell:" << sell_order_id
            << ", Price:" << price
            << ", Qty:" << quantity
            << "}";
        return oss.str();
    }

} // namespace OrderBook
