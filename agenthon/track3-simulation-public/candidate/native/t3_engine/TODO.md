# FastOrderBook implementation plan

The scaffold is complete when it builds and exposes guarded interfaces. The order-book milestone
is complete only when command-by-command differential replay and public semantic gates pass.

Local setup, build, test, sanitizer, and troubleshooting commands live in
[RUNBOOK.md](RUNBOOK.md).

## P0 — freeze the contract

- [ ] Enumerate every `OrderBook` method and attribute read by the pinned `ExchangeAgent`, trace
  adapter, shutdown path, and Track 3 patches.
- [ ] Capture ABIDES outcomes for validation failures, duplicate IDs, missing cancels, and every
  supported order flag.
- [ ] Define a versioned JSONL command/result schema for small committed fixtures.
- [ ] Add an AS01/AS06 boundary recorder without changing RNG consumption or event ordering.
- [ ] Implement a first-divergence report with command index, pre-state, expected result, actual
  result, and post-state.

## P1 — native state machine

- [ ] Implement limit insertion, crossing, multi-level sweeps, partial fills, and residual resting.
- [ ] Implement O(1)-after-lookup cancel and erase empty price levels.
- [ ] Implement market, partial-cancel, modify, and replace only after their ABIDES semantics are
  captured.
- [ ] Add price-to-comply and self-trade-prevention behavior required by Track 3 patches.
- [ ] Add L1/L2/L3 and transaction-volume query results required by `ExchangeAgent`.
- [ ] Replace mutation guard exceptions only when focused native tests exist.

## P2 — differential proof

- [ ] Unit-test FIFO, price priority, partial/full fill, level walking, cancel, and empty-book edges.
- [ ] Replay small adversarial fixtures through ABIDES and native books after every command.
- [ ] Replay recorded AS01 and AS06 streams with zero state/result mismatches.
- [ ] Run sanitizers and repeat-determinism checks.

## P3 — live integration

- [ ] Implement `candidate/abides_fork/fast_orderbook_adapter.py` translations.
- [ ] Preserve exact execution/accept/cancel message order and logging side effects.
- [ ] Build the wheel in the candidate Docker image without changing the baseline runtime path.
- [ ] Add an explicit switch for ABIDES versus native book during differential development.
- [ ] Pass all public Tier-A/Tier-B and message-ledger gates.

## P4 — performance

- [ ] Benchmark AS06 against 7,015.28 events/sec under comparable conditions.
- [ ] Re-profile before changing containers or allocation strategy.
- [ ] Consider intrusive/arena orders, dense prices, or flatter layouts only after measurement.
- [ ] Keep Python/C++ crossings at one call per complete command.
