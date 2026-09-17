import pytest

from t3_fast_orderbook import Order, OrderBook, Side


def test_empty_book() -> None:
    book = OrderBook()

    assert book.best_bid() is None
    assert book.best_ask() is None
    assert book.active_order_count == 0
    assert not book.contains(42)


def test_mutations_fail_until_semantics_are_implemented() -> None:
    book = OrderBook()
    order = Order(
        id=42,
        agent_id=7,
        side=Side.BID,
        price=10_000,
        quantity=5,
        timestamp=123,
    )

    with pytest.raises(RuntimeError, match="pinned-ABIDES differential contract"):
        book.add_limit_order(order)

