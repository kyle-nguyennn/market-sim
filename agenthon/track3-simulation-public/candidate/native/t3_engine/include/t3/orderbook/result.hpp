#pragma once

#include <vector>

#include "t3/orderbook/types.hpp"

namespace t3::orderbook {

struct BestQuote {
    Price price{};
    Quantity quantity{};
};

struct Fill {
    OrderId resting_order_id{};
    OrderId incoming_order_id{};
    AgentId resting_agent_id{};
    AgentId incoming_agent_id{};
    Price price{};
    Quantity quantity{};
};

struct MatchResult {
    std::vector<Fill> fills;
    Quantity remaining_quantity{};
    bool rested{false};
};

enum class CancelStatus : std::uint8_t {
    Cancelled = 0,
    NotFound = 1,
};

struct CancelResult {
    CancelStatus status{CancelStatus::NotFound};
    Quantity cancelled_quantity{};
};

}  // namespace t3::orderbook

