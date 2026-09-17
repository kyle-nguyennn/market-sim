# FastOrderBook assumptions

This document records provisional boundaries so the native implementation does not silently
invent behavior. ABIDES at the pinned Track 3 commit remains the executable specification; a
differential test overrides every assumption below.

## Native core boundary

- One `OrderBook` instance owns one symbol. Symbol validation remains in the Python compatibility
  layer because the native core receives no symbol strings.
- Prices, quantities, order IDs, agent IDs, and timestamps cross the Python boundary once as
  fixed-width integers. Their exact maximum ranges still need confirmation from public scenarios.
- The native book consumes no RNG and owns no event scheduling, latency, message delivery, or
  agent behavior.
- One mutation call is atomic from the caller's perspective. An incoming order may produce zero
  or more fills, returned in the exact sequence in which ABIDES would emit execution messages.
- Python owns conversion to ABIDES orders/messages, history rows, book logs, and exchange-side
  notifications until the later `FastExchange` stage.

## Provisional matching model

- Active levels use price priority; orders within one level use insertion FIFO.
- A match executes at the resting order's advertised price.
- A partially filled resting order keeps its queue position.
- The unfilled remainder of a non-marketable limit order rests at its limit price.
- Active order IDs are unique. Behavior for a duplicate active ID must be captured from ABIDES
  before implementation.
- Cancellation targets an active order by ID, with side/price treated as compatibility metadata.
  Exact not-found behavior must match ABIDES.

## Explicitly unresolved

- Market orders and the no-liquidity remainder.
- Partial cancel, modify, and replace queue-priority rules.
- Price-to-comply, hidden-order metadata, post-only tags, and other upstream order flags.
- The patched self-trade-prevention policies (`cancel_newest` and `cancel_oldest`).
- L1/L2/L3 query shapes, imbalance, transaction-volume windows, and last-trade aggregation.
- `quiet` behavior and the precise ordering of history, logging, owner callbacks, and messages.
- Whether every public/sealed scenario fits the initial integer widths and one-symbol-per-book
  model.

Do not resolve these from intuition. Record or construct a pinned-ABIDES case and add a
differential assertion first.

