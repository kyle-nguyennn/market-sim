# Bottleneck analysis — 2026-09-11

## Executive summary

Profiling AS01 and AS06 shows that much of the baseline's runtime goes into handling
messages and supporting work: formatting timestamps, copying log records, computing
liquidity statistics after trading ends, and constructing output tables. Order
matching matters, but the evidence does not justify treating the order book or event
queue as the only bottleneck.

The same major costs recur in both scenarios. AS06 produces about three times as
many market-event records, while most major function costs increase by about
2.7–2.9 times. The first optimization experiments should address lazy debug
formatting, pandas row iteration during shutdown, scalar latency clipping, and
redundant copying. These are proposed experiments; no speedup has been demonstrated
yet. Output semantics must remain unchanged.

## Setup and supporting artifacts

We used the Docker workflow in [the profiling guide](20260908_profile.md). The
[baseline Dockerfile](../baselines/Dockerfile) supplies Python 3.11, NumPy 1.26.4,
pandas 1.5.3, SciPy 1.17.1, PyArrow 15.0.2, and coloredlogs 15.0.1, plus ABIDES at
`f9cbe51342b7dedd9587e4e069040d68a5c6477f` with the four baseline patches. The
profiling derivative adds py-spy 0.4.2; cProfile runs in the original baseline image.
The documented run configuration uses four CPUs, 16 GiB memory, and no network.
The saved metadata does not independently record Docker resource settings or image
digests; future runs should capture those alongside the exact command.

Docker resolved the host setup issues encountered earlier: missing adapter/engine
imports and pandas 3 timestamp-resolution incompatibilities. A host ABIDES
installation is unnecessary for this workflow.

| Scenario | Public configuration | py-spy artifacts | cProfile artifacts |
|---|---|---|---|
| AS01 | [as01_base_mix.json](../regression_suite/scenarios/as01_base_mix.json) | [Flamegraph](../out/baseline_as01/profile.svg), [metadata](../out/baseline_as01/events.json) | [Cumulative log](../out/baseline_as01_cprofile/cumtime_call.log), [raw profile](../out/baseline_as01_cprofile/t3.prof), [metadata](../out/baseline_as01_cprofile/events.json) |
| AS06 | [as06_throughput_fast.json](../regression_suite/scenarios/as06_throughput_fast.json) | [Flamegraph](../out/baseline_as06/profile.svg), [metadata](../out/baseline_as06/events.json) | [Cumulative log](../out/baseline_as06_cprofile/cumtime_call.log), [raw profile](../out/baseline_as06_cprofile/t3.prof), [metadata](../out/baseline_as06_cprofile/events.json) |

Each run directory also contains `trace.parquet` and `message_trace.parquet`. These
links refer to local artifacts under the Git-ignored `out/` directory; sharing this
note alone does not share the profiles.

Both scenarios have a 20-second simulated horizon and 20 trading agents: 12 noise,
four value, two momentum, and two market makers. They use different scenario
settings and seeds: AS01 uses `1806084051`; AS06 uses `1040981657`. Their comparison
describes different workloads, not a before/after optimization experiment.

## Results and timing boundaries

| Measurement | AS01 | AS06 |
|---|---:|---:|
| Market-event trace rows | 24,695 | 74,502 |
| Message/wakeup ledger rows | 30,087 | 83,337 |
| ABIDES wall time under py-spy | 2.668 s | 7.637 s |
| Trace events/sec reported under py-spy | 9,257 | 9,755 |
| Samples represented in the SVG | 506 | 1,065 |
| ABIDES wall time under cProfile | 4.171 s | 11.779 s |
| cProfile total reported time | 6.097 s | 15.157 s |
| cProfile function calls, including recursion | 8,238,720 | 21,370,119 |

The timer in [simulate.py](../baselines/abides_fork/simulate.py) wraps
`abides.run(config)`. It includes kernel initialization, the event loop, and
termination. It excludes earlier configuration construction and later canonical
trace extraction and file writing. Both profilers observe the larger Python
process, including startup and imports. These local diagnostic numbers are not
official Runner throughput measurements.

The SVG sample totals are read from their `all` frames; AS01's SVG represents 506
samples even though its console reported 527. Use the SVG denominator when
interpreting its percentages. Flamegraph width includes descendants and is not a
chronological timeline. cProfile's `cumtime` includes callees, while `tottime`
excludes them; nested rows must not be added together. The printed `percall` values
often round to `0.000`, which does not mean the work is free.

cProfile's ABIDES times are about 54–56% higher than the corresponding py-spy runs.
Instrumentation overhead is a factor, but these separate executions do not isolate
its exact contribution. Use repeated unprofiled measurements to establish speedups.

### cProfile comparison

The following values come from the cumulative logs and raw profiles linked above.
Times are cumulative unless marked as self time.

| Function or phase | AS01 | AS06 | AS06 / AS01 |
|---|---:|---:|---:|
| `Kernel.runner` | 3.344 s | 9.414 s | 2.82× |
| Exchange `receive_message` | 1.410 s | 3.927 s | 2.79× |
| Adapter `receive_message` | 1.367 s | 3.879 s | 2.84× |
| `Kernel.terminate` | 0.825 s | 2.364 s | 2.87× |
| `OrderBook.handle_limit_order` | 0.816 s | 2.360 s | 2.89× |
| `get_time_dropout` | 0.707 s | 2.066 s | 2.92× |
| `deepcopy`, including recursive work | 0.621 s | 1.674 s | 2.69× |
| `extract_trace` | 0.541 s | 1.507 s | 2.79× |
| `get_latency` | 0.370 s | 1.012 s | 2.74× |
| `fmt_ts` self time | 0.323 s | 0.938 s | 2.90× |
| `extract_message_trace` | 0.217 s | 0.714 s | 3.30× |

AS06 has 3.02× as many trace rows and 2.77× as many ledger rows. Most major costs
grow roughly with activity, without an obvious new dominant path. Two scenarios
and one execution per profiler do not establish asymptotic scaling or statistical
significance. Message-trace extraction's 3.30× growth warrants more measurements.

## What the profiles establish

### Message handling dominates the event loop

Exchange and adapter message handlers together account for about 83% of event-loop
time in both runs. Their own bodies are much cheaper: exchange-handler self time
is 0.062/0.176 s for AS01/AS06, and adapter-handler self time is 0.019/0.052 s.
Similarly, limit-order handling has only 0.047/0.139 s self time despite its
0.816/2.360 s cumulative cost. Inspect the descendants rather than optimizing only
the dispatch wrappers. Sending messages, processing executions, logging, copying,
and latency calculations are nested within these paths.

### Shutdown liquidity analysis is substantial

`Kernel.terminate` consumes about 20% of the timed ABIDES run. Within it,
`get_time_dropout` accounts for about 17% of AS01's ABIDES time and 17.5% of AS06's.
The raw profiles identify its main costs:

| Callee of `get_time_dropout` | AS01 | AS06 |
|---|---:|---:|
| pandas `iterrows` | 0.458 s | 1.343 s |
| pandas Series indexing | 0.203 s | 0.586 s |

This function constructs a DataFrame from book history and walks rows to compute
periods without bid/ask liquidity. These are end-of-run metrics, but they still
fall inside the current ABIDES timer.

### Formatting and copying are recurring costs

`fmt_ts` is called 50,153 times in AS01 and 147,049 times in AS06. It consumes
0.323/0.938 s of self time. Order string representations account for 0.205/0.603 s
of that time; `Order.to_dict()` accounts for 0.118/0.335 s.

The inspected baseline source includes eager debug formatting, for example
`logger.debug(f"Received notification of execution for: {order}")`. Python
constructs this string before the logger decides whether to emit it. Not all
formatting is disposable: some supplies required event records.

`deepcopy` consumes 0.621/1.674 s cumulatively. Copies directly requested by
`Agent.logEvent` account for 0.346/0.925 s. The source also copies orders for agent
state and event serialization, so removing copying indiscriminately could make
historical records change when live orders are mutated.

### Scalar clipping costs more than the latency random draw

The [latency adapter](../baselines/abides_fork/config.py) is called 25,865 times in
AS01 and 72,515 times in AS06. Its cost decomposes as follows:

| Latency operation | AS01 | AS06 |
|---|---:|---:|
| Entire `get_latency` | 0.370 s | 1.012 s |
| `np.clip` call | 0.248 s | 0.674 s |
| Random lognormal draw | 0.050 s | 0.140 s |

NumPy's generic clipping machinery is prominent when used once per scalar message
latency. This supports testing scalar clipping without changing the random draws.

### Output construction matters more than Parquet writing here

Canonical event and message-trace extraction together consume 0.758 s in AS01 and
2.221 s in AS06 under cProfile. Within event extraction, `parse_logs_df` takes
0.419/1.150 s. The py-spy profiles also show substantial extraction work; the two
Parquet write call sites together occupy only about 2.2%/0.7% of represented
AS01/AS06 samples. Improving output construction can reduce full-process runtime
without changing the current `events.json` ABIDES timer.

### Message counts describe activity, not CPU shares

In AS06's [message ledger](../out/baseline_as06/message_trace.parquet), spread queries
and responses contribute 21,560 records, or 25.9% of the ledger. Execution messages
contribute 25,204; limit-order messages contribute 14,655. Frequent queries are a
candidate for more detailed measurement, but their frequency does not establish
their CPU share. Ledger timestamps and latency fields describe simulated time,
not profiling durations.

## Suggested optimization areas

Priorities below reflect how focused an experiment can be, its observed cost, and
the difficulty of preserving behavior. Profile totals are not promised savings.

| Priority | Area | First experiment | Validation needed |
|---|---|---|---|
| 1 | Eager debug formatting | Use lazy logger arguments, such as `logger.debug("Received notification of execution for: %s", order)`, and guard other expensive debug-only expressions. | Same simulation traces and required event records; check debug output when enabled. |
| 2 | Shutdown row iteration | Compute liquidity durations directly from book records or equivalent arrays, avoiding per-row pandas Series construction. | Same metrics for empty books, bid/ask liquidity transitions, repeated timestamps, and terminal intervals. |
| 3 | Scalar latency clipping | Replace scalar `np.clip` with equivalent scalar comparisons. | Same sampled values, clipping boundaries, rounding, integer conversion, RNG state, and message ordering. |
| 4 | Redundant copying | Identify duplicate snapshots in `logEvent` and order serialization; copy only data that must remain independent. | Historical logs remain immutable under later order mutation; fills and message-ledger content remain unchanged. |
| 5 | Trace construction | Reduce repeated conversion and intermediate objects in `parse_logs_df`, `extract_trace`, and `extract_message_trace`. | Same schemas, integer timestamp precision, null handling, stable row order, event classification, and ledger causality. |
| 6 | Matching and scheduling | Reprofile after the focused changes; inspect remaining order-book and queue costs before redesigning them. | Full semantic regression, including priority, partial fills, cancellations, and causal ordering. |

The current evidence does not establish heap operations as the dominant cost, nor
does it establish a need for GPU acceleration. Required logging or ledger work
cannot simply be removed to make the profiler look faster. Native costs may also
be attributed to Python callers in the existing py-spy runs, which did not request
native-stack profiling.

## Correctness evidence and next measurements

All eight saved Parquet files were checked against the SHA-256 values in their
respective `events.json`. For each scenario, py-spy and cProfile runs produced
identical event-trace and message-trace hashes. This establishes consistency of
these saved outputs across the two profilers; it is not a replacement for regression
against the expected simulator semantics. The AS01 console also contained warnings
about fills for orders absent from an agent's outstanding-order list. They did not
prevent completion, but their cause has not been resolved by this profiling work.

For each optimization experiment:

1. Change one area and rebuild the baseline and profiling images. Record source
   revision, image digest, scenario/seed, Docker settings, profiler options, and the
   exact command. Use a separate folder, for example
   `out/lazy_logging_as01_run01/`.
2. Compare baseline and candidate outputs for both scenarios and run the public
   regression workflow described in [the baseline README](../baselines/README.md).
   For intended behavior-preserving changes, investigate any hash difference with
   semantic comparisons; serialization differences alone need not imply a market
   behavior difference.
3. Collect repeated unprofiled runs under identical resource settings on an
   otherwise-idle host. Report both full-process elapsed time and ABIDES time,
   including variation across runs. Keep runs sequential.
4. Reprofile to verify that the targeted cost decreased and identify the next
   limiting path. In the cProfile browser, use the commands below; callers/callees
   resolve costs hidden by the top-40 cumulative report.

```text
sort cumulative
stats 40
sort time
stats 40
callers fmt_ts
callers deepcopy
callees get_time_dropout
callees get_latency
callees handle_limit_order
```
