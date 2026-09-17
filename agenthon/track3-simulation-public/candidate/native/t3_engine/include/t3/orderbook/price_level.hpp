#pragma once

#include <list>

#include "t3/orderbook/order.hpp"

namespace t3::orderbook {

struct PriceLevel {
    Price price{};
    Quantity total_quantity{};
    std::list<Order> orders;
};

}  // namespace t3::orderbook

