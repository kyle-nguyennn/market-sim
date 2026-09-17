#include <cassert>
#include <stdexcept>

#include "t3/orderbook/order_book.hpp"

int main() {
    using namespace t3::orderbook;

    OrderBook book;
    assert(!book.best_bid().has_value());
    assert(!book.best_ask().has_value());
    assert(book.active_order_count() == 0);
    assert(!book.contains(42));

    bool mutation_is_guarded = false;
    try {
        static_cast<void>(
            book.add_limit_order(Order{42, 7, Side::Bid, 10'000, 5, 123})
        );
    } catch (const std::logic_error&) {
        mutation_is_guarded = true;
    }
    assert(mutation_is_guarded);

    book.clear();
    return 0;
}

