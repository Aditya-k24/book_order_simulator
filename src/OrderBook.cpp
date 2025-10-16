/**
 * @file OrderBook.cpp
 * @brief OrderBook class implementation
 */

#include "OrderBook.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace OrderBook {

    OrderBook::OrderBook(const std::string& symbol) 
        : symbol_(symbol)
    {
    }

    bool OrderBook::addOrder(std::shared_ptr<Order> order) {
        if (!order) return false;
        
        std::unique_lock<std::mutex> lock(book_mutex_);
        
        // Add to order map for O(1) lookup
        orders_[order->getId()] = order;
        
        // Add to appropriate price level
        PriceLevelMap& price_map = getPriceLevelMap(order->getSide());
        auto it = price_map.find(order->getPrice());
        
        if (it != price_map.end()) {
            it->second.addOrder(order);
        } else {
            PriceLevel level(order->getPrice());
            level.addOrder(order);
            price_map[order->getPrice()] = level;
        }
        
        return true;
    }

    bool OrderBook::cancelOrder(Order::OrderID order_id) {
        std::unique_lock<std::mutex> lock(book_mutex_);
        
        auto order_it = orders_.find(order_id);
        if (order_it == orders_.end()) {
            return false;
        }
        
        auto order = order_it->second;
        PriceLevelMap& price_map = getPriceLevelMap(order->getSide());
        auto level_it = price_map.find(order->getPrice());
        
        if (level_it != price_map.end()) {
            bool removed = level_it->second.removeOrder(order_id);
            if (removed && level_it->second.isEmpty()) {
                removeEmptyPriceLevel(order->getSide(), order->getPrice());
            }
        }
        
        orders_.erase(order_it);
        return true;
    }

    uint64_t OrderBook::getBestBid() const {
        std::unique_lock<std::mutex> lock(book_mutex_);
        if (bids_.empty()) return 0;
        return bids_.rbegin()->first; // Highest bid
    }

    uint64_t OrderBook::getBestAsk() const {
        std::unique_lock<std::mutex> lock(book_mutex_);
        if (asks_.empty()) return 0;
        return asks_.begin()->first; // Lowest ask
    }

    uint64_t OrderBook::getSpread() const {
        uint64_t best_bid = getBestBid();
        uint64_t best_ask = getBestAsk();
        if (best_bid == 0 || best_ask == 0) return 0;
        return best_ask - best_bid;
    }

    uint64_t OrderBook::getBestBidQuantity() const {
        std::unique_lock<std::mutex> lock(book_mutex_);
        if (bids_.empty()) return 0;
        return bids_.rbegin()->second.total_quantity;
    }

    uint64_t OrderBook::getBestAskQuantity() const {
        std::unique_lock<std::mutex> lock(book_mutex_);
        if (asks_.empty()) return 0;
        return asks_.begin()->second.total_quantity;
    }

    std::shared_ptr<Order> OrderBook::getOrder(Order::OrderID order_id) const {
        std::unique_lock<std::mutex> lock(book_mutex_);
        auto it = orders_.find(order_id);
        return (it != orders_.end()) ? it->second : nullptr;
    }

    std::vector<std::shared_ptr<Order>> OrderBook::getOrdersAtPrice(uint64_t price, OrderSide side) const {
        std::unique_lock<std::mutex> lock(book_mutex_);
        const PriceLevelMap& price_map = getPriceLevelMap(side);
        auto it = price_map.find(price);
        
        if (it != price_map.end()) {
            return it->second.orders;
        }
        return {};
    }

    std::pair<std::vector<std::pair<uint64_t, uint64_t>>, 
              std::vector<std::pair<uint64_t, uint64_t>>> 
    OrderBook::getMarketDepth(size_t levels) const {
        std::unique_lock<std::mutex> lock(book_mutex_);
        
        std::vector<std::pair<uint64_t, uint64_t>> bid_levels;
        std::vector<std::pair<uint64_t, uint64_t>> ask_levels;
        
        // Get top bid levels (highest prices first)
        auto bid_it = bids_.rbegin();
        for (size_t i = 0; i < levels && bid_it != bids_.rend(); ++i, ++bid_it) {
            bid_levels.emplace_back(bid_it->first, bid_it->second.total_quantity);
        }
        
        // Get top ask levels (lowest prices first)
        auto ask_it = asks_.begin();
        for (size_t i = 0; i < levels && ask_it != asks_.end(); ++i, ++ask_it) {
            ask_levels.emplace_back(ask_it->first, ask_it->second.total_quantity);
        }
        
        return {std::move(bid_levels), std::move(ask_levels)};
    }

    size_t OrderBook::getOrderCount() const {
        std::unique_lock<std::mutex> lock(book_mutex_);
        return orders_.size();
    }

    bool OrderBook::isEmpty() const {
        std::unique_lock<std::mutex> lock(book_mutex_);
        return orders_.empty();
    }

    void OrderBook::clear() {
        std::unique_lock<std::mutex> lock(book_mutex_);
        bids_.clear();
        asks_.clear();
        orders_.clear();
    }

    std::string OrderBook::toString(size_t levels) const {
        std::unique_lock<std::mutex> lock(book_mutex_);
        
        std::ostringstream oss;
        oss << "\n=== Order Book: " << symbol_ << " ===\n";
        
        auto [bid_levels, ask_levels] = getMarketDepth(levels);
        
        // Print asks (highest first)
        oss << "ASKS:\n";
        for (auto it = ask_levels.rbegin(); it != ask_levels.rend(); ++it) {
            oss << std::setw(8) << it->first << " | " << std::setw(10) << it->second << "\n";
        }
        
        // Print spread line
        uint64_t spread = getSpread();
        oss << "--------|------------\n";
        oss << "SPREAD: " << spread << "\n";
        oss << "--------|------------\n";
        
        // Print bids (highest first)
        oss << "BIDS:\n";
        for (const auto& level : bid_levels) {
            oss << std::setw(8) << level.first << " | " << std::setw(10) << level.second << "\n";
        }
        
        oss << "\nTotal Orders: " << orders_.size() << "\n";
        oss << "==================\n";
        
        return oss.str();
    }

    std::pair<uint64_t, uint64_t> OrderBook::getBestPrices() const {
        return {getBestBid(), getBestAsk()};
    }

    std::vector<std::shared_ptr<Order>> OrderBook::getOrdersForMatching(OrderSide side) const {
        std::unique_lock<std::mutex> lock(book_mutex_);
        const PriceLevelMap& price_map = getPriceLevelMap(side);
        
        if (price_map.empty()) return {};
        
        // Get orders from best price level
        if (side == OrderSide::BUY) {
            auto best_level_it = price_map.rbegin();
            return best_level_it->second.orders;
        } else {
            auto best_level_it = price_map.begin();
            return best_level_it->second.orders;
        }
    }

    void OrderBook::updateOrderQuantity(Order::OrderID order_id, uint64_t old_qty, uint64_t new_qty) {
        std::unique_lock<std::mutex> lock(book_mutex_);
        
        auto order_it = orders_.find(order_id);
        if (order_it == orders_.end()) return;
        
        auto order = order_it->second;
        PriceLevelMap& price_map = getPriceLevelMap(order->getSide());
        auto level_it = price_map.find(order->getPrice());
        
        if (level_it != price_map.end()) {
            level_it->second.updateQuantity(order_id, old_qty, new_qty);
        }
    }

    OrderBook::PriceLevelMap& OrderBook::getPriceLevelMap(OrderSide side) {
        return (side == OrderSide::BUY) ? bids_ : asks_;
    }

    const OrderBook::PriceLevelMap& OrderBook::getPriceLevelMap(OrderSide side) const {
        return (side == OrderSide::BUY) ? bids_ : asks_;
    }

    void OrderBook::removeEmptyPriceLevel(OrderSide side, uint64_t price) {
        PriceLevelMap& price_map = getPriceLevelMap(side);
        auto it = price_map.find(price);
        if (it != price_map.end() && it->second.isEmpty()) {
            price_map.erase(it);
        }
    }

} // namespace OrderBook
