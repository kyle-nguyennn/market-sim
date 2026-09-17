"""Compatibility boundary between pinned ABIDES and the native order-book core.

This module is intentionally not wired into ``simulate.py`` yet. The pinned ABIDES
``OrderBook`` remains active until every method/attribute used by ``ExchangeAgent`` and the
trace path has a recorded differential contract.
"""

from typing import Any

from t3_fast_orderbook import OrderBook as NativeOrderBook


class FastOrderBookAdapter:
    """Own a native book while preserving the future ABIDES-facing boundary.

    TODO: add ABIDES methods only after their behavior is captured. The known surface includes
    limit/market orders, cancel/partial-cancel/modify/replace, L1/L2/L3 queries, imbalance,
    transacted volume, history, last trade, and logging timestamps.
    """

    def __init__(self, owner: Any, symbol: str) -> None:
        self.owner = owner
        self.symbol = symbol
        self._native = NativeOrderBook()

    @property
    def native(self) -> NativeOrderBook:
        """Expose the native instance to differential tooling only."""

        return self._native

    def handle_limit_order(self, order: Any, quiet: bool = False) -> None:
        """Translate one ABIDES limit order after message/log ordering is specified."""

        del order, quiet
        raise NotImplementedError(
            "TODO: capture ABIDES result and side-effect ordering before integration"
        )

