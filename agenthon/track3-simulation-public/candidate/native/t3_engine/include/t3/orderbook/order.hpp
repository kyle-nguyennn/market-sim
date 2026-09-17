#pragma once

#include "t3/orderbook/types.hpp"

namespace t3::orderbook {

struct Order {
    OrderId id{};
    AgentId agent_id{};
    Side side{Side::Bid};
    Price price{};
    Quantity quantity{};
    TimeNs timestamp{};
};

}  // namespace t3::orderbook

