# Profile analysis: baseline AS01 and AS06

The saved profiles show substantial work in agent and exchange message handling,
plus shutdown analysis and trace construction. AS06 is the better starting point
because it has more samples and less startup overhead relative to simulation work.
These are observations from two different scenarios, not a before/after speed test.

## Run summary

| Run | Reported ABIDES time | Trace events | Ledger records | Trace events/sec | Samples represented in SVG |
|---|---:|---:|---:|---:|---:|
| [baseline_as01](baseline_as01/profile.svg) | 2.668 s | 24,695 | 30,087 | 9,257 | 506 |
| [baseline_as06](baseline_as06/profile.svg) | 7.637 s | 74,502 | 83,337 | 9,755 | 1,065 |

Counts and timing come from each run's `events.json`; the Parquet row counts agree.
Sample totals come from the SVG's `all` frame, which need not equal the profiler's
console sample count. The reported time wraps `abides.run(config)`: it includes
kernel initialization, the event loop, and termination. It excludes configuration
building, subsequent canonical trace extraction, and file writing. The py-spy
profile covers the whole Python process.

## Where the samples go

The following non-overlapping categories use the caller frames at each phase
boundary. Percentages refer to all samples represented in that run's SVG; they are
sampling estimates, not exact measured durations.

| Phase | AS01 | AS06 |
|---|---:|---:|
| Kernel event-loop call, including its descendants | 46.4% | 54.8% |
| Kernel termination call, including its descendants | 6.5% | 13.9% |
| Canonical event-trace extraction | 12.3% | 12.4% |
| Message-ledger extraction | 5.1% | 7.1% |
| Two Parquet write calls | 2.2% | 0.7% |
| Startup, configuration, and other work | 27.5% | 11.1% |

Trace construction is more prominent than the actual Parquet writes. Improving
trace extraction can reduce full-process runtime without changing the time reported
in `events.json`, because extraction happens after that timer stops.

## Where to inspect first in AS06

These function-level percentages aggregate all sampled source lines and call paths.
Repeated recursive appearances are counted once per sampled stack. Values are
inclusive of callees and overlap: do not add them together.

| Function or path | All-process samples | What to investigate |
|---|---:|---|
| Adapter `receive_message` | 23.8% | Parent trading-agent handling and subsequent actions |
| Exchange `receive_message` | 23.3% | Limit-order processing and response generation |
| `OrderBook.handle_limit_order` | 14.5% | Matching, execution messages, and book logging |
| Shutdown `analyse_order_book` / `get_time_dropout` | 10.1% | Iteration over book history using pandas `iterrows` |
| `Kernel.send_message` | 9.7% | Scheduling and latency calculation |

Additional overlapping paths include `get_latency` (7.0%), `append_book_log2`
(5.7%), and `copy.deepcopy` (4.8%). This profile does not establish heap operations
as the dominant cost. Native work may be attributed to its Python caller because
the command did not request native-stack profiling.

The shutdown path is a concrete candidate for investigation: its
`get_time_dropout` implementation builds a DataFrame from book history and iterates
through rows to compute liquidity metrics. Any optimization must preserve those
metrics and the required trace outputs.

## Reading the SVGs

1. Open `baseline_as06/profile.svg` in a browser. Hover a rectangle to see its
   source location and sample count; click it to zoom, and use Reset Zoom to return.
2. Width represents sampled stack frequency. Vertical position represents call
   depth; horizontal position is not a timeline. Colors do not indicate severity.
3. Search for `runner`, `receive_message`, `handle_limit_order`,
   `get_time_dropout`, or `extract_trace` using the graph's Search control.
4. Inspect descendants to find where a wide parent spends its time. A wide
   `receive_message` frame does not mean that function's own statements are costly.
5. The same function can appear at several source lines and call paths. A single
   rectangle can therefore show less than the aggregate percentages above.

## What the Parquet files add

`trace.parquet` contains market-event output; `message_trace.parquet` contains the
message/wakeup ledger. Their `msg_type` counts describe workload composition, not
CPU cost per type. In AS06:

- Execution messages: 25,204 records.
- Limit-order messages: 14,655 records.
- Spread queries plus responses: 21,560 records, or 25.9% of ledger records.
- Wakeups: 10,822 records.

Frequent spread queries are worth investigating, but a 25.9% record share does not
mean they consume 25.9% of CPU time. Message timestamps and `latency_ns` represent
simulated market time, not profiler durations.

## Next measurements

Run cProfile for AS06 using the Docker commands in
`../notes/20260908_profile.md`, with `run_name=baseline_as06_cprofile` and
`scenario_file=as06_throughput_fast.json`. In pstats, inspect:

```text
sort cumulative
stats 30
sort time
stats 30
callers get_time_dropout
callees handle_limit_order
```

`cumtime` includes callees; `tottime` excludes them; `ncalls` gives call counts.
There are currently no cProfile `.prof` files in these output folders.

For an optimization comparison, keep the scenario, seed, image source, CPU/memory
limits, and profiler settings fixed. Retain repeated runs in distinct folders and
compare unprofiled timings as well, especially when using cProfile. Capture the
image ID and exact command with future runs: the present metadata does not record
them. Validate output semantics with the regression harness before treating a
faster implementation as a successful change.
