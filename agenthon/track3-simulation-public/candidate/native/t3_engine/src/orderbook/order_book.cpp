#include "t3/orderbook/order_book.hpp"

#include <stdexcept>

namespace t3::orderbook {

MatchResult OrderBook::add_limit_order(const Order&) {
    throw std::logic_error(
        "TODO: implement only after the pinned-ABIDES differential contract is captured"
    );
}

CancelResult OrderBook::cancel(OrderId) {
    throw std::logic_error(
        "TODO: implement only after the pinned-ABIDES cancel contract is captured"
    );
}

std::optional<BestQuote> OrderBook::best_bid() const noexcept {
    if (bids_.empty()) {
        return std::nullopt;
    }
    const auto& [price, level] = *bids_.begin();
    return BestQuote{price, level.total_quantity};
}

std::optional<BestQuote> OrderBook::best_ask() const noexcept {
    if (asks_.empty()) {
        return std::nullopt;
    }
    const auto& [price, level] = *asks_.begin();
    return BestQuote{price, level.total_quantity};
}

bool OrderBook::contains(OrderId order_id) const noexcept {
    return orders_.contains(order_id);
}

std::size_t OrderBook::active_order_count() const noexcept {
    return orders_.size();
}

void OrderBook::clear() noexcept {
    orders_.clear();
    bids_.clear();
    asks_.clear();
}

}  // namespace t3::orderbook

