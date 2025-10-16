/**
 * @file MatchingEngine.cpp
 * @brief Matching engine implementation
 */

#include "MatchingEngine.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace OrderBook {

    MatchingEngine::MatchingEngine(const std::string& symbol)
        : symbol_(symbol)
        , order_book_(symbol)
        , trade_count_(0)
        , total_volume_(0)
        , total_value_(0)
        , csv_logging_enabled_(false)
    {
    }

    MatchingEngine::~MatchingEngine() {
        if (csv_file_.is_open()) {
            csv_file_.close();
        }
    }

    bool MatchingEngine::submitOrder(std::shared_ptr<Order> order) {
        if (!order) return false;
        
        // Try to match the order first
        matchOrder(order);
        
        // If order wasn't completely filled, add to book
        if (!order->isFilled()) {
            order_book_.addOrder(order);
            notifyOrderCallback(order);
        }
        
        return true;
    }

    bool MatchingEngine::cancelOrder(Order::OrderID order_id) {
        bool cancelled = order_book_.cancelOrder(order_id);
        if (cancelled) {
            notifyOrderCallback(order_book_.getOrder(order_id));
        }
        return cancelled;
    }

    void MatchingEngine::setCSVLogging(bool enable, const std::string& filename) {
        csv_logging_enabled_ = enable;
        csv_filename_ = filename;
        
        if (enable) {
            std::lock_guard<std::mutex> lock(csv_mutex_);
            csv_file_.open(filename, std::ios::out | std::ios::app);
            if (csv_file_.is_open()) {
                // Write header if file is new/empty
                csv_file_.seekp(0, std::ios::end);
                if (csv_file_.tellp() == 0) {
                    csv_file_ << "timestamp,buyOrderID,sellOrderID,price,quantity\n";
                }
            }
        } else {
            std::lock_guard<std::mutex> lock(csv_mutex_);
            if (csv_file_.is_open()) {
                csv_file_.close();
            }
        }
    }

    std::string MatchingEngine::getOrderBookSnapshot(size_t levels) const {
        return order_book_.toString(levels);
    }

    std::string MatchingEngine::getMarketStats() const {
        std::ostringstream oss;
        oss << "\n=== Market Statistics ===\n";
        oss << "Symbol: " << symbol_ << "\n";
        oss << "Total Trades: " << trade_count_.load() << "\n";
        oss << "Total Volume: " << total_volume_.load() << "\n";
        oss << "Total Value: " << total_value_.load() << "\n";
        oss << "Active Orders: " << order_book_.getOrderCount() << "\n";
        
        uint64_t best_bid = order_book_.getBestBid();
        uint64_t best_ask = order_book_.getBestAsk();
        uint64_t spread = order_book_.getSpread();
        
        oss << "Best Bid: " << best_bid << " (Qty: " << order_book_.getBestBidQuantity() << ")\n";
        oss << "Best Ask: " << best_ask << " (Qty: " << order_book_.getBestAskQuantity() << ")\n";
        oss << "Spread: " << spread << "\n";
        
        if (trade_count_.load() > 0) {
            oss << "Average Trade Price: " << (total_value_.load() / total_volume_.load()) << "\n";
        }
        
        oss << "========================\n";
        return oss.str();
    }

    void MatchingEngine::clear() {
        order_book_.clear();
        trades_.clear();
        trade_count_.store(0);
        total_volume_.store(0);
        total_value_.store(0);
    }

    size_t MatchingEngine::processBatch(const std::vector<std::shared_ptr<Order>>& orders) {
        size_t processed = 0;
        for (const auto& order : orders) {
            if (submitOrder(order)) {
                processed++;
            }
        }
        return processed;
    }

    size_t MatchingEngine::matchOrder(std::shared_ptr<Order> order) {
        size_t trades_executed = 0;
        
        while (!order->isFilled()) {
            // Get opposing orders for matching
            OrderSide opposing_side = (order->getSide() == OrderSide::BUY) ? 
                                    OrderSide::SELL : OrderSide::BUY;
            
            auto opposing_orders = order_book_.getOrdersForMatching(opposing_side);
            
            if (opposing_orders.empty()) {
                break; // No more orders to match against
            }
            
            // Find best matching order (price-time priority)
            std::shared_ptr<Order> best_match = nullptr;
            uint64_t best_price = 0;
            
            for (const auto& opposing_order : opposing_orders) {
                if (!opposing_order || opposing_order->isFilled()) continue;
                
                // Check if prices are compatible
                bool price_compatible = false;
                if (order->getSide() == OrderSide::BUY && 
                    opposing_order->getSide() == OrderSide::SELL &&
                    order->getPrice() >= opposing_order->getPrice()) {
                    price_compatible = true;
                    best_price = opposing_order->getPrice();
                } else if (order->getSide() == OrderSide::SELL && 
                          opposing_order->getSide() == OrderSide::BUY &&
                          order->getPrice() <= opposing_order->getPrice()) {
                    price_compatible = true;
                    best_price = opposing_order->getPrice();
                }
                
                if (price_compatible && 
                    (!best_match || opposing_order->getTimestamp() < best_match->getTimestamp())) {
                    best_match = opposing_order;
                }
            }
            
            if (!best_match) {
                break; // No compatible orders found
            }
            
            // Execute trade
            uint64_t trade_quantity = std::min(order->getRemainingQuantity(), 
                                             best_match->getRemainingQuantity());
            
            executeTrade(order, best_match, best_price, trade_quantity);
            
            // Update order quantities
            order->reduceQuantity(trade_quantity);
            best_match->reduceQuantity(trade_quantity);
            
            // Update order book quantities
            order_book_.updateOrderQuantity(order->getId(), 
                                          order->getRemainingQuantity() + trade_quantity,
                                          order->getRemainingQuantity());
            order_book_.updateOrderQuantity(best_match->getId(),
                                          best_match->getRemainingQuantity() + trade_quantity,
                                          best_match->getRemainingQuantity());
            
            // Remove filled orders from book
            if (best_match->isFilled()) {
                order_book_.cancelOrder(best_match->getId());
                notifyOrderCallback(best_match);
            }
            
            trades_executed++;
        }
        
        return trades_executed;
    }

    Trade MatchingEngine::executeTrade(std::shared_ptr<Order> buy_order, 
                                     std::shared_ptr<Order> sell_order,
                                     uint64_t trade_price, 
                                     uint64_t trade_quantity) {
        // Ensure buy order is actually the buy side
        if (buy_order->getSide() != OrderSide::BUY) {
            std::swap(buy_order, sell_order);
        }
        
        auto now = std::chrono::high_resolution_clock::now();
        Trade trade(buy_order->getId(), sell_order->getId(), 
                   trade_price, trade_quantity, now);
        
        // Store trade
        trades_.push_back(trade);
        
        // Update statistics
        trade_count_.fetch_add(1);
        total_volume_.fetch_add(trade_quantity);
        total_value_.fetch_add(trade_price * trade_quantity);
        
        // Log trade
        logTradeToCSV(trade);
        notifyTradeCallback(trade);
        
        // Print trade to console
        std::cout << "TRADE: " << trade.toString() << std::endl;
        
        return trade;
    }

    void MatchingEngine::logTradeToCSV(const Trade& trade) {
        if (!csv_logging_enabled_) return;
        
        std::lock_guard<std::mutex> lock(csv_mutex_);
        if (csv_file_.is_open()) {
            csv_file_ << trade.toCSV() << "\n";
            csv_file_.flush();
        }
    }

    void MatchingEngine::notifyTradeCallback(const Trade& trade) {
        if (trade_callback_) {
            trade_callback_(trade);
        }
    }

    void MatchingEngine::notifyOrderCallback(std::shared_ptr<Order> order) {
        if (order_callback_) {
            order_callback_(order);
        }
    }

} // namespace OrderBook
