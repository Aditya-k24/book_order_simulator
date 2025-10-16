/**
 * @file Order.cpp
 * @brief Order class implementation
 */

#include "Order.h"
#include <sstream>
#include <iomanip>

namespace OrderBook {

    Order::Order(OrderID id, OrderSide side, uint64_t price, uint64_t quantity, TimePoint timestamp)
        : id_(id)
        , side_(side)
        , price_(price)
        , quantity_(quantity)
        , remaining_quantity_(quantity)
        , timestamp_(timestamp)
        , type_(OrderType::LIMIT)
    {
    }

    std::string Order::toString() const {
        std::ostringstream oss;
        oss << "Order{ID:" << id_ 
            << ", Side:" << (side_ == OrderSide::BUY ? "BUY" : "SELL")
            << ", Price:" << price_ 
            << ", Qty:" << quantity_
            << ", Remaining:" << remaining_quantity_
            << ", Type:" << (type_ == OrderType::LIMIT ? "LIMIT" : "MARKET")
            << "}";
        return oss.str();
    }

} // namespace OrderBook
