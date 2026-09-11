# Bottleneck analysis — 2026-09-11

## Executive summary

**Our optimization objective is to increase `median_events_per_sec` reported by
[`throughput/timer.py`](../throughput/timer.py), while preserving the required
simulation output.** For a fixed scenario configuration and seed, the required
market-event count is fixed. We improve throughput by producing those events in
less host-measured wall-clock time. Use unprofiled timer runs to judge success;
use py-spy and cProfile to explain costs and choose experiments.

The established AS06 baseline is **7,015.28 events/sec**, measured by
`throughput/timer.py` over four retained runs after one warm-up. The
[saved throughput log](../out/baseline_as06/throughput.log) is the reference for
subsequent AS06 comparisons under matching conditions.

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

`throughput/timer.py` measures the whole container invocation, so shutdown,
formatting, trace construction, and output writing all contribute to its runtime.
A lower `Kernel.runner` time or a better rate inside the container's `events.json`
is supporting evidence; the comparison metric is the timer's output.

## Optimization objective and event-count definition

The local timer computes each run's rate and the overall result as:

```text
run_events_per_sec = n_events / host_wall_clock_sec
median_events_per_sec = median(run_events_per_sec for retained runs)
```

In `run_single`, the timer reads `n_events` from the container's `events.json`.
The baseline [simulate.py](../baselines/abides_fork/simulate.py) writes that value
as `int(len(trace))`: the row count of the canonical market-event table saved to
`trace.parquet`. It counts submissions, acceptances, cancellations, replacements,
fills, partial fills, and quote updates. It does not count every kernel message,
wakeup, log entry, or function call. The separate `n_messages`/message-ledger count
is not the throughput numerator.

For the specific scenario/seed pairs profiled here, the required output contains
24,695 market events for AS01 and 74,502 for AS06. Keeping that count and the required
semantics unchanged while halving a run's host wall time doubles its throughput.
All supporting work still contributes to the denominator, even if it creates no
additional counted events. Do not inflate the count or omit required output to
improve the reported rate.

**Fixed count means fixed configuration AND seed.** `measure_throughput` derives a
deterministic seed family from the scenario's `base_seed` (falling back to `seed`)
and runs the image once per derived seed. Different repeats can therefore have
different event counts, including counts different from the profiles above. Match
the scenario file, seed family, repeat count, warm-up policy, and resource settings
between baseline and candidate. Compare their medians of per-run rates; do not
divide the original profile's event count by a timer run's elapsed time or replace
the metric with total events divided by total time.

The timer's numerator is locally self-reported: it does not independently recount
the Parquet rows. Validate the count and simulation semantics separately. The
[README's official scoring description](../README.md#how-throughput-is-measured)
uses Runner-counted rows and trusted timing; the local timer is our development
comparison tool, not an official score. That distinction does not change the
objective for the experiments in this note.

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

## Established AS06 throughput baseline

Source: [out/baseline_as06/throughput.log](../out/baseline_as06/throughput.log).
This is a completed `throughput/timer.py` measurement of
`track3-abides-baseline:latest` on `as06_throughput_fast.json`. Five container runs
were performed; the first was discarded as warm-up and four contributed to the
summary.

| Metric | Result |
|---|---:|
| **Primary metric: `median_events_per_sec`** | **7,015.28 events/sec** |
| Mean of retained rates | 7,024.53 events/sec |
| Population standard deviation of retained rates | 181.96 events/sec |
| Minimum / maximum retained rate | 6,805.26 / 7,262.29 events/sec |
| Retained-run host wall-clock range | 10.200–10.950 s |
| Retained runs / total runs | 4 / 5 |

The individual measurements are:

| Run | Use | Seed | Host wall-clock time | Events/sec |
|---|---|---:|---:|---:|
| 1 | Warm-up; excluded | 481425548 | 11.693 s | 6,383.79 |
| 2 | Retained | 2141560590 | 10.950 s | 6,805.26 |
| 3 | Retained | 2065014021 | 10.200 s | 7,262.29 |
| 4 | Retained | 2097829639 | 10.493 s | 7,132.88 |
| 5 | Retained | 546074625 | 10.753 s | 6,897.68 |

The exact saved median is `7015.281130007508`. It is the median of the four
retained per-run rates, with each rate using that seed's event count and the
host-measured container duration. Do not substitute the original profile's 74,502
rows for each timer repeat: this batch uses a derived seed family.

Use this result as the initial AS06 comparison baseline:

```text
AS06 speedup = candidate.median_events_per_sec / 7015.281130007508
```

Match the scenario, seed family, repeat/warm-up policy, resource limits, and host
conditions. The standard deviation is about 2.6% of the mean across these four
runs; it describes this batch, not a confidence interval or a universal threshold
for accepting a speedup. Repeat baseline and candidate batches before drawing
conclusions about small differences.

The approximately 9,755 events/sec in the AS06 py-spy run's `events.json` is a
separate internal-timer diagnostic. **7,015.28 events/sec is the established local
throughput measurement to optimize against.** Their difference cannot be assigned
solely to profiler overhead: their timing boundaries and seed sets also differ.
The throughput log records the image tag but not its immutable digest, host
fingerprint, or explicit CPU/memory options, so preserve those with future runs.

## Profiling results and timing boundaries

| Measurement | AS01 | AS06 |
|---|---:|---:|
| Market-event trace rows | 24,695 | 74,502 |
| Message/wakeup ledger rows | 30,087 | 83,337 |
| ABIDES wall time under py-spy | 2.668 s | 7.637 s |
| Baseline's internal events/sec under py-spy | 9,257 | 9,755 |
| Samples represented in the SVG | 506 | 1,065 |
| ABIDES wall time under cProfile | 4.171 s | 11.779 s |
| cProfile total reported time | 6.097 s | 15.157 s |
| cProfile function calls, including recursion | 8,238,720 | 21,370,119 |

The timer in [simulate.py](../baselines/abides_fork/simulate.py) wraps
`abides.run(config)`. It includes kernel initialization, the event loop, and
termination. It excludes earlier configuration construction and later canonical
trace extraction and file writing. Both profilers observe the larger Python
process, including startup and imports. The table contains profiling diagnostics;
the internal rates are not `throughput/timer.py` results. Record the timer's
`median_events_per_sec` separately when assessing an optimization.

### Which work affects the timer's result?

In `timed_container_run`, the measurement surrounds the bounded container call:

```python
t_start = time.monotonic()
proc = bounded_container_run(cmd, cidfile=cidfile, timeout_sec=timeout_sec)
wall_clock = time.monotonic() - t_start
```

The call launches `docker run` and waits for completion. Sampler setup/teardown is
outside these timestamps; reading `events.json` on the host happens afterward.
Work inside the invocation has no exemption because it is called `terminate`,
logging, or serialization. Discarding the first warm-up run excludes that run from
the aggregate; it does not subtract startup or shutdown from later runs.

| Work | Inside baseline `events.json` timer? | Inside `throughput/timer.py` measurement? |
|---|---|---|
| Event dispatch, matching, latency | Yes | Yes |
| `Kernel.terminate`, including liquidity analysis | Yes | Yes |
| Debug formatting during simulation/termination | Yes | Yes |
| Canonical trace/ledger extraction | No | Yes |
| Parquet and metadata writes in the container | No | Yes |
| Imports/configuration in the container | No | Yes |

Optimize any significant cost in this interval that can be reduced while keeping
the same correct output. The exact official Runner boundary is outside this
checkout; it is unnecessary to speculate about that boundary when comparing
baseline and candidate with this concrete local timer.

This also separates **required behavior** from **incidental implementation work**:
the gates evaluate the output contract and simulation semantics, not preservation
of every ABIDES internal diagnostic. An unused shutdown diagnostic could potentially
be omitted after proving that no required output, later shutdown consumer, or
supported interface depends on it. Do not remove `Kernel.terminate` wholesale or
disable event logging: shutdown returns the state used for extraction, and
`extract_trace` reads agent logs through `parse_logs_df`. Lazy debug formatting is
a narrower change than suppressing those required records.

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
AS01/AS06 samples. Improving output construction can improve
`throughput/timer.py`'s `median_events_per_sec` without changing the current
`events.json` ABIDES timer. Judge the experiment by the former.

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
Success for every area below means improved `median_events_per_sec` from
unprofiled `throughput/timer.py` runs, with unchanged required events and semantics.
Shutdown and formatting remain in scope because they delay container completion.
Trace construction also remains a substantial candidate (2.221 s in AS06 under
cProfile); its position below the smaller, more isolated experiments is about
implementation scope, not exclusion from the measurement.

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
3. Run the unprofiled timer commands below for baseline and candidate under
   identical resource settings on an otherwise-idle host. Match the seed families
   and keep runs sequential. Compare `median_events_per_sec`, retaining
   `raw_events_per_sec`, `wall_clock_seconds`, and `std_events_per_sec` to assess
   variation. ABIDES time and profile percentages are secondary diagnostics.
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

### Measure the primary metric with `throughput/timer.py`

From the repository root, measure the baseline image using five runs, discarding
the first as warm-up. The host runs the timer; the simulator runs inside Docker.
These commands launch no profiler, and `--output` creates its parent directory.

```bash
image=track3-abides-baseline:latest
variant=baseline

python throughput/timer.py \
    --image "$image" \
    --scenario regression_suite/scenarios/as01_base_mix.json \
    --runs 5 --discard-warmup --cpus 4 --memory 16g \
    --output "out/${variant}_as01_throughput/throughput.json"

python throughput/timer.py \
    --image "$image" \
    --scenario regression_suite/scenarios/as06_throughput_fast.json \
    --runs 5 --discard-warmup --cpus 4 --memory 16g \
    --output "out/${variant}_as06_throughput/throughput.json"
```

After building a candidate image, repeat the commands with its image tag and a new
`variant`, for example `lazy_logging`. Use distinct variant suffixes for repeated
batches you want to retain. Preserve the scenario files and all timer options.
Confirm the saved `seed_family` and `warmup_discarded` agree before comparing.

For each scenario, report:

```text
speedup = candidate.median_events_per_sec / baseline.median_events_per_sec
```

Only call the optimization successful when this metric improves consistently
across repeated batches and correctness checks still pass. A lower cProfile time,
fewer function calls, or a higher rate in the simulator's own `events.json` is
insufficient on its own. The timer normally removes its temporary simulation
outputs, so retain correctness runs separately rather than assuming the summary
JSON also preserves their traces.
