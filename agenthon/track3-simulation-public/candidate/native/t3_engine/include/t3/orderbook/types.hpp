#pragma once

#include <cstdint>

namespace t3::orderbook {

using OrderId = std::uint64_t;
using AgentId = std::uint32_t;
using Price = std::int64_t;
using Quantity = std::uint32_t;
using TimeNs = std::int64_t;

enum class Side : std::uint8_t {
    Bid = 0,
    Ask = 1,
};

}  // namespace t3::orderbook

