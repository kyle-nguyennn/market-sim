# T3 optimization plan — invasive simulator replacement pivot

_Last updated: 2026-09-16_

## Executive summary

The T3 optimization strategy is pivoting away from adapter-level micro-optimization.
The new objective is to treat ABIDES as the **executable semantic specification** and
progressively replace its expensive deterministic internals with a purpose-built,
high-performance implementation.

The optimization target remains:

```text
maximize median_events_per_sec
subject to semantic correctness = PASS
```

The established local AS06 baseline remains **7,015.28 events/sec** from
`throughput/timer.py` under the previously recorded conditions. Use repeated,
unprofiled timer runs for performance decisions. Use profiles and differential tests
to explain costs and validate equivalence.

The first invasive replacement target is the **ABIDES OrderBook**, implemented as a
C++ `FastOrderBook` exposed through a thin Python/pybind11 compatibility layer. The
ABIDES kernel, event scheduling, agent wakeups, latency model, and seeded NumPy RNG
streams remain unchanged during this first phase.

The staged replacement path is now:

```text
Stage 0  ABIDES baseline as oracle
Stage 1  ABIDES Kernel + ABIDES ExchangeAgent + C++ FastOrderBook
Stage 2  ABIDES Kernel + FastExchange + C++ FastOrderBook
Stage 3  FastKernel + FastExchange + FastOrderBook + ABIDES-compatible RNG boundary
Stage 4  collapse object-heavy compatibility layers and optimize output path
```

The strategic principle is:

> Preserve stochastic behavior and externally visible semantics; replace deterministic
> machinery aggressively behind validated boundaries.

---

## 1. Why the strategy changed

The previous plan prioritized low-risk Python and adapter optimizations such as lazy
formatting, pandas cleanup, redundant copying, and trace construction. Those remain
valid optimizations, but they are no longer the main technical direction.

Reasons for the pivot:

1. The competition permits arbitrary internal architecture as long as the required
   CLI/output contract and semantic gates are preserved.
2. The supplied ABIDES implementation is a baseline engine, not an architectural
   requirement.
3. Profiling shows substantial time inside the actual simulator stack:
   `Kernel.runner`, `ExchangeAgent.receive_message`,
   `OrderBook.handle_limit_order`, message delivery, copying, and logging.
4. Optimizing only the adapter has limited upside and does not attack the central
   systems problem of the track.
5. T3 is more naturally interpreted as: **reimplement the same market mechanism more
   efficiently**, not "make this Python adapter slightly faster."

Adapter/output micro-optimizations are now secondary cleanup work after the native
execution path is established.

---

## 2. Baseline facts retained from profiling

### Primary local comparison metric

For AS06, the established baseline is:

```text
median_events_per_sec = 7015.281130007508
```

Reference formula:

```text
AS06 speedup = candidate.median_events_per_sec / 7015.281130007508
```

Match scenario, seed family, repeat/warm-up policy, resource limits, and host
conditions before interpreting a speedup.

### Profiling summary

Representative AS06 costs previously measured include:

```text
Kernel.runner                         ~9.4 s cumulative under cProfile
ExchangeAgent.receive_message         ~3.9 s cumulative
adapter receive_message               ~3.9 s cumulative
OrderBook.handle_limit_order          ~2.36 s cumulative
Kernel.terminate                      ~2.36 s cumulative
get_time_dropout                      ~2.07 s cumulative
copy.deepcopy                         ~1.67 s cumulative
extract_trace                         ~1.51 s cumulative
get_latency                           ~1.01 s cumulative
fmt_ts                                ~0.94 s self time
extract_message_trace                 ~0.71 s cumulative
```

These cumulative numbers overlap and must not be summed. They establish where to
look, not independent phase percentages.

The key implication for the new plan is that the Exchange/OrderBook path is large,
stateful, deterministic, and separable enough to replace without immediately
rewriting stochastic behavior.

---

## 3. ABIDES is now the executable specification

Do not preserve ABIDES internal architecture merely because it exists.

For each component, identify its externally observable state transition and reproduce
that behavior more efficiently.

For the order book, the conceptual contract is:

\[
(B, C) \rightarrow (B', F, R)
\]

where:

- `B` = current book state,
- `C` = incoming command,
- `B'` = resulting book state,
- `F` = ordered execution/fill sequence,
- `R` = ordered protocol-visible responses / book changes.

Implementation details inside ABIDES are not sacred. Observable semantics are.

The baseline should therefore be treated as an oracle for:

- price-time priority,
- partial-fill behavior,
- execution price,
- cancellation semantics,
- modification/replacement semantics,
- self-trade prevention behavior,
- order identifiers,
- fill ordering,
- response ordering,
- book state transitions,
- timing-sensitive protocol effects.

---

## 4. Stochastic boundary: preserve first, replace later

The RNG layer is intentionally **not** part of the first rewrite.

The current Track 3 baseline creates deterministic child `np.random.RandomState`
streams from the scenario seed. Randomness is concentrated in a few boundaries:

### Configuration seed fan-out

Keep the existing fixed child-stream construction order.

### NoiseTrader

Each action consumes stochastic values for roughly:

```text
order size
side
price offset
```

### ValueTrader / oracle observation

The stochastic input enters through oracle observation using the trader's seeded
`RandomState`.

### Latency model

Stochastic latency consumes RNG per qualifying message. This is the most sensitive
boundary because event/message ordering determines which draw maps to which message.

### Deterministic agents

MarketMaker and MomentumTrader behavior is deterministic given the observed state.

### Rule

During the OrderBook replacement:

```text
DO NOT change RNG implementation
DO NOT change event ordering
DO NOT change message scheduling
DO NOT change latency draw ordering
DO NOT change agent wakeup behavior
```

This keeps the first invasive experiment focused on deterministic state transitions.

---

## 5. Stage 0 — freeze ABIDES as the oracle

Before implementing the native book, preserve a known-good baseline.

Required artifacts:

- baseline commit/tag,
- AS01/AS06 traces,
- fill sequence,
- message trace,
- throughput logs,
- semantic regression results,
- representative protocol scenarios.

### Differential tooling

We need two levels of differential testing.

#### A. Whole-simulator differential

Compare:

```text
baseline trace.parquet
candidate trace.parquet
baseline message_trace.parquet
candidate message_trace.parquet
```

Report the first semantic divergence.

#### B. OrderBook boundary replay

Instrument the boundary between ExchangeAgent and OrderBook and record a replayable
command stream such as:

```text
ADD LIMIT
CANCEL
MODIFY / REPLACE
QUERY / SNAPSHOT where required for validation
```

Replay the same commands into:

```text
ABIDES OrderBook
C++ FastOrderBook
```

After every command compare:

- ordered fills,
- best bid/ask,
- remaining quantity,
- active-order set,
- price-level ordering,
- FIFO order within levels,
- any externally required book/log event.

Desired failure output:

```text
command index: 38192
command: LIMIT BID id=1298 px=100003 qty=7
first mismatch: fill sequence
baseline: ...
candidate: ...
book before: ...
book after baseline: ...
book after candidate: ...
```

This harness is a hard prerequisite for aggressive book optimization.

---

## 6. Stage 1 — C++ FastOrderBook behind ABIDES ExchangeAgent

### Architecture

```text
ABIDES Kernel
      |
      v
ABIDES ExchangeAgent
      |
      | Python ABIDES Order / command
      v
FastOrderBookAdapter.py
      |
      | primitive fields
      v
C++ FastOrderBook (pybind11)
      |
      | compact MatchResult / BookResult
      v
FastOrderBookAdapter.py
      |
      v
ABIDES-compatible execution/messages/log state
```

### Why this boundary

This preserves:

- ABIDES kernel scheduling,
- message ordering,
- computation delays,
- latency model and RNG draw order,
- agent behavior,
- ExchangeAgent protocol behavior.

Only deterministic book state and matching are replaced.

That gives the first-stage invariant:

```text
same incoming book command + same book state
=> same resulting book state + same ordered executions
```

### Language / binding

Use **C++ + pybind11** for the initial native implementation.

Do not manipulate Python objects inside the matching loop. Convert at the boundary to
primitive native fields.

Illustrative native records:

```cpp
struct Order {
    uint64_t order_id;
    uint32_t agent_id;
    int64_t price;
    uint32_t quantity;
    uint8_t side;
    int64_t timestamp;
};

struct Fill {
    uint64_t resting_order_id;
    uint64_t incoming_order_id;
    uint32_t resting_agent_id;
    uint32_t incoming_agent_id;
    int64_t price;
    uint32_t quantity;
};
```

One Python -> C++ call should process an entire book command, including all matches
across price levels. Do not cross the language boundary per individual match step.

---

## 7. FastOrderBook v1 data structures

Start with semantic clarity before exotic optimization.

Suggested v1:

```text
bids: ordered price -> PriceLevel
asks: ordered price -> PriceLevel
order_id -> OrderHandle
PriceLevel -> FIFO linked/list-like order queue
```

Reasonable initial C++ implementation:

```text
std::map for ordered active price levels
std::unordered_map for direct order-id lookup
stable FIFO container / intrusive links for orders at a level
```

Required complexity targets:

```text
best price        O(1) from begin()/cached handle
order lookup      expected O(1)
cancel            expected O(1) after lookup
FIFO pop          O(1)
partial decrement O(1)
```

Do not optimize away tree lookup until benchmark evidence says it matters.

Potential later representations:

- dense price arrays if tick range is bounded,
- flat ordered vectors if active levels are small,
- intrusive arena-allocated orders,
- custom slab/arena allocator,
- struct-of-arrays representation,
- specialized integer price indexing.

Every representation change must remain behind the differential harness.

---

## 8. Preserve semantics, not the Python object model

The Python compatibility layer should preserve the subset of the ABIDES OrderBook
surface required by ExchangeAgent and the regression suite.

Do **not** port ABIDES Python classes one-for-one into C++.

Bad native boundary:

```text
C++ repeatedly reading/writing py::object fields during matching
```

Preferred boundary:

```text
Python ABIDES object
    -> extract primitive fields once
    -> native match/update
    -> return compact result once
    -> construct required ABIDES-visible responses
```

This phase intentionally tolerates Python conversion overhead because the native core
must first prove semantic equivalence. Later stages remove more of that boundary.

---

## 9. Logging and book history

Do not blindly reproduce ABIDES' object-heavy logging path inside the native book.

The profile showed meaningful cost from:

- copying,
- book logging,
- shutdown analysis,
- trace extraction.

The native core should maintain compact mutation/event records where possible:

```text
ADD
FILL
PARTIAL_FILL
CANCEL
REPLACE
LEVEL_CREATED
LEVEL_REMOVED
```

Only materialize Python/logging structures needed by the current ExchangeAgent and
required output.

However, do not delete baseline log state until we have proved that:

1. no Tier-A/Tier-B semantic check depends on it,
2. trace extraction does not depend on it,
3. shutdown/statistical metrics do not depend on it.

The first goal is native matching equivalence, not simultaneous logging redesign.

---

## 10. FastOrderBook semantic checklist

Before performance claims, verify all of these against ABIDES:

- [ ] same-price FIFO
- [ ] price priority
- [ ] incoming aggressive order walks levels identically
- [ ] partial fill
- [ ] full fill
- [ ] residual incoming quantity rests correctly
- [ ] execution price matches ABIDES
- [ ] cancel active order
- [ ] cancel partially filled order
- [ ] cancel nonexistent/already-completed order behavior
- [ ] replace/modify semantics
- [ ] order ID preservation
- [ ] self-trade prevention behavior
- [ ] empty-book behavior
- [ ] best bid/ask transitions
- [ ] level deletion when depth reaches zero
- [ ] multiple executions from one incoming order preserve exact order
- [ ] quantities and integer prices preserve exact types/ranges
- [ ] book-visible state after every command matches

Add targeted adversarial unit scenarios rather than relying only on whole-run traces.

---

## 11. Stage 1 success gate

Do not proceed to ExchangeAgent replacement until all of the following hold:

```text
FastOrderBook replay differential: PASS
Tier-A public semantic regression: PASS
Tier-B required semantics/statistical gates: PASS
repeat determinism: PASS
AS06 throughput: measured improvement or clear hot-path shift
```

If the C++ book is semantically exact but total throughput gain is small, keep it if
it creates the correct foundation for Stage 2; profile again before deciding whether
the Python ExchangeAgent/message layer is now dominant.

---

## 12. Stage 2 — replace ExchangeAgent

Once the book is exact, absorb ExchangeAgent protocol work into a lightweight
implementation.

Target architecture:

```text
ABIDES Kernel
      |
      v
FastExchange
      |
      v
C++ FastOrderBook
```

Preserve externally visible protocol semantics:

- receive/send ordering,
- accepted/rejected behavior,
- spread queries,
- execution responses,
- cancellation/replacement responses,
- computation delay semantics,
- self-trade prevention,
- market open/close behavior,
- message ordering after multi-fill commands.

The goal is to eliminate ABIDES object and dispatch overhead around the already-proven
native matching core.

---

## 13. Stage 3 — replace Kernel / event engine

Kernel replacement happens only after Exchange + OrderBook are stable.

The fast kernel must reproduce the Track 3 subset of:

- event priority queue,
- timestamp ordering,
- deterministic tie-breaking,
- wakeups,
- message scheduling,
- computation delay,
- latency application,
- sender/recipient semantics,
- termination boundaries.

### RNG constraint

Keep the existing stochastic streams at first.

The most dangerous coupling is:

```text
message/event order
    -> latency RNG consumption order
    -> future delivery times
    -> future event order
```

A single ordering difference can cascade through the entire run.

Therefore the kernel rewrite should include optional debug instrumentation for:

```text
(event timestamp, tie-break sequence, sender, recipient, message type)
latency RNG call index/value
agent RNG call counters where useful
```

Use this only for differential debugging, not benchmark runs.

---

## 14. Revised optimization priority

Old priority:

```text
adapter micro-optimization
-> Python cleanup
-> maybe native rewrite later
```

New priority:

```text
1. differential harness / executable specification
2. C++ FastOrderBook
3. integrate under ABIDES ExchangeAgent
4. validate semantics and benchmark
5. FastExchange
6. validate and benchmark
7. FastKernel with preserved RNG boundary
8. collapse Python object model / trace path
9. data-layout and allocator optimization
10. only then consider deeper RNG/native stochastic replacement
```

Adapter formatting / pandas / serialization improvements may still be harvested, but
they should not distract from the core replacement path.

---

## 15. Branching strategy

Use isolated invasive branches.

Current planned branch:

```text
feat/t3-fast-orderbook
```

Purpose:

```text
C++ FastOrderBook
+ pybind11 binding
+ Python compatibility adapter
+ order-book differential replay harness
+ semantic tests
+ benchmark integration
```

Do not mix Kernel replacement into this branch until the OrderBook stage is proven.

Suggested later branches:

```text
feat/t3-fast-exchange
feat/t3-fast-kernel
perf/t3-native-output
```

Each stage must be mergeable/revertible independently.

---

## 16. Immediate implementation sequence

### Milestone A — understand exact ABIDES OrderBook surface

- [ ] enumerate methods called by ExchangeAgent
- [ ] enumerate attributes read by ExchangeAgent / shutdown / trace code
- [ ] document command semantics
- [ ] document fill/result semantics
- [ ] document book logging dependencies
- [ ] document STP patch behavior

### Milestone B — boundary recorder

- [ ] record real AS01/AS06 OrderBook command stream
- [ ] serialize primitive command fields
- [ ] capture expected result after each command
- [ ] capture optional compact book snapshot/hash

### Milestone C — C++ skeleton

- [ ] pybind11 build integrated into candidate Docker image
- [ ] native order/price-level structures
- [ ] limit-order insertion
- [ ] matching across levels
- [ ] partial fills
- [ ] cancel
- [ ] replace/modify if required by ABIDES surface

### Milestone D — differential equivalence

- [ ] replay AS01 stream
- [ ] replay AS06 stream
- [ ] protocol adversarial tests
- [ ] zero mismatches

### Milestone E — live integration

- [ ] wire adapter into ExchangeAgent
- [ ] run whole scenario
- [ ] run public regression suite
- [ ] compare first divergence if any

### Milestone F — benchmark

- [ ] repeat AS06 `throughput/timer.py`
- [ ] compare median to 7,015.28 events/sec
- [ ] re-profile candidate
- [ ] identify new dominant path
- [ ] decide whether to optimize native book further or move to FastExchange

---

## 17. Performance discipline

For every candidate:

```text
Hypothesis:
Change:
Semantic result:
Baseline metric:
Candidate metric:
Speedup:
Profile shift:
Decision:
```

Do not optimize C++ internals based on intuition alone. A native rewrite can still be
slow if dominated by:

- Python/C++ object conversion,
- excessive allocations,
- cache-unfriendly containers,
- compatibility logging,
- surrounding ExchangeAgent/message overhead.

Measure after each integration step.

---

## 18. Kill / rollback criteria

Rollback or isolate an approach if:

- exact fill ordering cannot be reproduced,
- order IDs diverge,
- repeated runs become nondeterministic,
- wrapper complexity grows into a second ABIDES implementation,
- native/Python crossings occur in inner matching loops,
- semantic regressions become opaque,
- performance does not improve and the architecture does not unlock the next stage.

A difficult semantic bug is not automatically a reason to abandon the native book;
use the differential harness to localize it to the first divergent command.

---

## 19. Current working thesis

The intended high-value solution is likely closer to a specialized simulator than to
an optimized adapter.

Our working decomposition is:

```text
KEEP EXACT INITIALLY
--------------------
scenario interpretation
seed fan-out
NumPy RandomState behavior
agent stochastic policy
latency RNG behavior
event/message ordering

REPLACE PROGRESSIVELY
---------------------
OrderBook
ExchangeAgent internals
Kernel/event machinery
Python object-heavy state
logging/copy-heavy paths
trace/output materialization
```

The first invasive bet is deliberately the component with the cleanest deterministic
boundary:

```text
ABIDES ExchangeAgent
        ->
C++ FastOrderBook
```

If that boundary proves exact, it becomes the anchor for replacing the rest of the
ABIDES stack without conflating matching semantics, event semantics, and RNG
compatibility in one rewrite.

---

## 20. Next action

Start development on `feat/t3-fast-orderbook`.

The first code task is **not** "write the fastest book." It is:

> Build the smallest native order-book implementation and differential replay harness
> that can prove command-by-command equivalence with ABIDES.

Once equivalence is established, optimize the data structure and broaden the
replacement boundary.