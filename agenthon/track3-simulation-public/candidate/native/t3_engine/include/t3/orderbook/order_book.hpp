#pragma once

#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>

#include "t3/orderbook/order.hpp"
#include "t3/orderbook/price_level.hpp"
#include "t3/orderbook/result.hpp"

namespace t3::orderbook {

class OrderBook {
  public:
    OrderBook() = default;

    [[nodiscard]] MatchResult add_limit_order(const Order& order);
    [[nodiscard]] CancelResult cancel(OrderId order_id);

    [[nodiscard]] std::optional<BestQuote> best_bid() const noexcept;
    [[nodiscard]] std::optional<BestQuote> best_ask() const noexcept;
    [[nodiscard]] bool contains(OrderId order_id) const noexcept;
    [[nodiscard]] std::size_t active_order_count() const noexcept;

    void clear() noexcept;

  private:
    using BidLevels = std::map<Price, PriceLevel, std::greater<Price>>;
    using AskLevels = std::map<Price, PriceLevel>;

    struct OrderHandle {
        Side side{};
        Price price{};
        std::list<Order>::iterator order;
    };

    BidLevels bids_;
    AskLevels asks_;
    std::unordered_map<OrderId, OrderHandle> orders_;
};

}  // namespace t3::orderbook

