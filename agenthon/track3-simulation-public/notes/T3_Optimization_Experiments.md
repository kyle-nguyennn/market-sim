# Track 3 Optimization Experiments

## Purpose

This document defines the execution protocol for optimizing the Agenthon 2026 Track 3 ABIDES simulator while preserving simulation semantics.

The goal is to make every optimization:

- isolated,
- measurable,
- reproducible,
- attributable,
- correctness-preserving,
- easy for a coding agent to execute without contaminating the baseline.

The baseline implementation is immutable. Every experiment starts from the same candidate-control commit and changes exactly one optimization area.

---

## Core invariant

For any behavior-preserving optimization:

```text
same scenario + same seed
    =>
same externally observable simulation semantics
```

The candidate may change **how** the simulation is executed, but not **what market behavior is produced**.

For early micro-optimizations, the preferred standard is stronger:

```text
same seed
    =>
identical canonical trace
and
identical message ledger
```

where possible.

---

# 1. Repository structure

The Track 3 public repository currently contains the pinned ABIDES baseline under:

```text
agenthon/
└── track3-simulation-public/
    ├── baselines/
    │   ├── Dockerfile
    │   └── abides_fork/
    ├── regression_suite/
    ├── throughput/
    ├── notes/
    └── out/
```

The contents of `baselines/` are the immutable control and MUST NOT be edited.

Create a separate candidate implementation:

```text
agenthon/
└── track3-simulation-public/
    ├── baselines/                   # IMMUTABLE reference
    │   ├── Dockerfile
    │   └── abides_fork/
    │
    ├── candidate/                   # optimized implementation
    │   ├── Dockerfile
    │   └── abides_fork/
    │       ├── config.py
    │       ├── agents.py
    │       ├── trace.py
    │       ├── simulate.py
    │       └── ...
    │
    ├── experiments/
    │   ├── README.md
    │   ├── results/
    │   │   ├── E00_candidate_control.md
    │   │   ├── E01_scalar_latency_clip.md
    │   │   ├── E02_lazy_debug_logging.md
    │   │   └── ...
    │   └── scripts/
    │       ├── compare_outputs.py
    │       ├── run_correctness.sh
    │       └── run_benchmark.sh
    │
    ├── regression_suite/
    ├── throughput/
    ├── notes/
    └── out/
```

## Rule

Never modify:

```text
baselines/
```

All candidate code lives under:

```text
candidate/
```

The candidate-control branch must initially be a functional copy of the baseline implementation.

---

# 2. Git branch strategy

## Base branches

Use:

```text
main
```

for the original repository state.

Create:

```text
exp/t3-candidate-control
```

from `main`.

This branch creates the candidate folder and build path but introduces **no intentional semantic or performance optimization**.

All experiment branches MUST branch from the same candidate-control commit.

```text
main
 │
 └── exp/t3-candidate-control
      │
      ├── exp/t3-scalar-latency-clip
      ├── exp/t3-lazy-debug-logging
      ├── exp/t3-remove-dropout-analysis
      ├── exp/t3-vectorize-dropout-analysis
      ├── exp/t3-logevent-copy-elision
      ├── exp/t3-parse-logs-fastpath
      ├── exp/t3-trace-construction
      ├── exp/t3-message-trace-construction
      ├── exp/t3-order-serialization
      └── exp/t3-import-startup
```

Experiments MUST NOT branch from one another.

Bad:

```text
candidate-control
    -> E01
        -> E02
            -> E03
```

Good:

```text
candidate-control
    -> E01

candidate-control
    -> E02

candidate-control
    -> E03
```

This ensures:

```text
measured delta = effect of exactly one optimization
```

---

# 3. Integration branch

Create a separate branch:

```text
t3/integration
```

This branch contains only optimizations that have independently passed:

1. correctness,
2. public regression,
3. repeated throughput measurement.

Successful experiment commits should be cherry-picked into `t3/integration`.

Example:

```text
candidate-control
    + E01 PASS
    + E02 PASS
    + E03 PASS
    + E07 PASS
```

Do not assume isolated speedups compose linearly.

In general:

```text
speedup(E01 + E02) != speedup(E01) + speedup(E02)
```

After each integration cherry-pick:

- rerun correctness,
- rerun throughput,
- record cumulative speedup.

---

# 4. Experiment lifecycle

Every experiment follows the same sequence.

```text
candidate-control
    ↓
create experiment branch
    ↓
make exactly one optimization
    ↓
optimization-specific unit tests
    ↓
AS01 differential correctness
    ↓
AS06 differential correctness
    ↓
full public regression
    ↓
unprofiled benchmark
    ↓
reprofile only if needed
    ↓
KEEP / REJECT
```

Do not benchmark seriously before correctness passes.

Do not collect another broad flamegraph unless:

- the current optimization behaves unexpectedly, or
- the current optimization set has materially changed the hotspot distribution.

---

# 5. Correctness protocol

Correctness is not:

```text
"the simulator still runs"
```

Correctness is:

```text
candidate preserves required observable behavior
```

Use the following gates.

## C0 — output contract

Candidate must produce:

```text
trace.parquet
message_trace.parquet
events.json
```

with the expected schemas and dtypes.

---

## C1 — fixed-seed differential comparison

Run baseline and candidate using exactly the same:

- scenario,
- seed,
- resource limits,
- simulator configuration.

Compare:

```text
baseline(S, seed)
candidate(S, seed)
```

For behavior-neutral micro-optimizations, prefer exact logical equality of:

```text
trace.parquet
message_trace.parquet
```

Do not rely only on SHA equality.

Parquet byte representation may differ while the logical table remains identical.

Use logical DataFrame comparison as the semantic requirement.

Example:

```python
pd.testing.assert_frame_equal(
    baseline_trace,
    candidate_trace,
    check_dtype=True,
    check_exact=True,
)
```

---

## C2 — semantic invariants

Verify, for the same seed:

```text
same event count
same fill count
same fill sequence
same prices
same quantities
same order IDs
same cancellations
same replacements
same quote sequence
same event ordering
```

For the message ledger:

```text
same seq
same t_recv_ns
same t_send_ns
same latency_ns
same src_id
same dst_id
same message_id
same msg_type
same order_id
same causal_parent
```

Also verify:

```text
t_recv_ns - t_send_ns == latency_ns
```

where applicable.

---

## C3 — full public regression

After AS01 and AS06 differential tests pass, run the full public regression suite.

An optimization is not accepted merely because AS01 and AS06 pass.

Public scenarios may exercise:

- price-time priority,
- latency ordering,
- partial fills,
- cancellations,
- replacement behavior,
- protocol behavior,
- reactive agents,
- statistical families.

---

## C4 — experiment-specific invariants

Each experiment must define its own additional correctness assertions.

Examples:

### Scalar latency clipping

Must preserve:

```text
random draw count
random draw order
clipped value
rounding
integer conversion
RNG state
message ordering
```

Test RNG alignment explicitly:

```python
rs_old = np.random.RandomState(seed)
rs_new = np.random.RandomState(seed)

old_value = old_latency(rs_old, ...)
new_value = new_latency(rs_new, ...)

assert old_value == new_value
assert rs_old.randint(...) == rs_new.randint(...)
```

### Copy-elision

Must prove that later mutation of a live order cannot mutate historical logs.

### Trace fast path

Must preserve:

```text
event classification
timestamp precision
row ordering
null behavior
dtypes
canonical schema
```

### Matching-engine optimization

Must preserve:

```text
price priority
time priority
partial-fill semantics
cancel semantics
replace semantics
execution price
causal ordering
```

---

# 6. Benchmark protocol

The primary metric is:

```text
median_events_per_sec
```

from the local Track 3 throughput harness.

The internal simulator `events.json` rate is diagnostic only.

## Screening run

For each experiment:

```text
5 total runs
1 warm-up discarded
4 retained
```

Use identical:

- scenario,
- seed family,
- CPU limit,
- memory limit,
- host,
- Docker runtime,
- warm-up policy.

Benchmark both:

```text
AS01
AS06
```

---

## Paired measurement

For promising changes, use paired/interleaved runs:

```text
B(seed1)
C(seed1)
B(seed2)
C(seed2)
B(seed3)
C(seed3)
...
```

where:

```text
B = candidate-control
C = experiment candidate
```

Record:

```text
official-like speedup
=
median(candidate EPS)
/
median(control EPS)
```

and per-seed diagnostic ratios:

```text
r_i
=
EPS_candidate(seed_i)
/
EPS_control(seed_i)
```

A small speedup near the noise floor must be repeated before being accepted.

---

# 7. Acceptance rule

An experiment is accepted only if:

```text
C0 PASS
AND
C1 PASS
AND
C2 PASS
AND
C3 PASS
AND
C4 PASS
AND
measurable throughput improvement
```

In symbolic form:

```text
KEEP
=
correctness
∧ regression
∧ measurable speedup
```

If correctness fails:

```text
REJECT
```

regardless of performance.

If performance improvement is within noise:

```text
INCONCLUSIVE
```

and rerun.

---

# 8. Experiment matrix

| ID | Branch | Isolated change | Main hypothesis | Correctness focus |
|---|---|---|---|---|
| E00 | `exp/t3-candidate-control` | Candidate copy only | Candidate layout/build introduces no semantic or meaningful timing change | Exact output equivalence |
| E01 | `exp/t3-scalar-latency-clip` | `np.clip` scalar → direct scalar branch | NumPy scalar dispatch is unnecessary overhead | RNG state + exact latency/order |
| E02 | `exp/t3-lazy-debug-logging` | Lazy logger arguments | Eager formatting is paid even when debug is disabled | Same simulation + equivalent debug behavior |
| E03 | `exp/t3-remove-dropout-analysis` | Remove unused shutdown liquidity analysis | Track 3 does not consume these metrics | Prove metric tracker is externally unused |
| E04 | `exp/t3-vectorize-dropout-analysis` | Replace pandas `iterrows` path | If E03 is unsafe, compute same metric more efficiently | Exact metric equivalence |
| E05 | `exp/t3-logevent-copy-elision` | Remove redundant deep copies | Snapshot/log copies are over-conservative | Historical immutability |
| E06 | `exp/t3-parse-logs-fastpath` | Track-3-specific log extraction | Generic `parse_logs_df` creates unnecessary objects | Exact canonical trace |
| E07 | `exp/t3-trace-construction` | Reduce DataFrame/intermediate construction | Trace extraction can be cheaper | Schema/order/dtypes |
| E08 | `exp/t3-message-trace-construction` | Reduce message-ledger conversion overhead | Ledger materialization is expensive | Exact causal ordering |
| E09 | `exp/t3-order-serialization` | Avoid unnecessary `__str__` / `to_dict` work | Serialization occurs in hot paths unnecessarily | Required logs unchanged |
| E10 | `exp/t3-import-startup` | Reduce candidate-only startup/import cost | Whole container lifecycle is timed | Same CLI/output |
| E11+ | `exp/t3-<specific-hotspot>` | Created only after reprofile | Remaining hotspot is worth structural optimization | Full semantic suite |

---

# 9. Mutually exclusive experiments

E03 and E04 are alternatives.

First test:

```text
E03: remove dropout analysis
```

If E03 is fully correctness-safe, do not optimize code that no longer needs to run.

Only run:

```text
E04: vectorize dropout analysis
```

if E03 proves unsafe because the metric is externally required.

Principle:

```text
eliminate unnecessary work
    >
make unnecessary work faster
```

---

# 10. Experiment result record

Every experiment branch must add one result file:

```text
experiments/results/E##_name.md
```

Template:

```markdown
# E## — Experiment Name

## Metadata

Branch:
Commit:
Parent commit:
Candidate image:
Baseline image:

## Hypothesis

...

## Code change

...

## Correctness

| Check | Result |
|---|---|
| Optimization-specific unit tests | PASS / FAIL |
| AS01 trace | EXACT / DIFFERENT |
| AS01 message trace | EXACT / DIFFERENT |
| AS06 trace | EXACT / DIFFERENT |
| AS06 message trace | EXACT / DIFFERENT |
| Public regression | PASS / FAIL |

## Performance

| Scenario | Control EPS | Candidate EPS | Speedup |
|---|---:|---:|---:|
| AS01 | | | |
| AS06 | | | |

Paired ratios:

```text
seed1:
seed2:
seed3:
...
```

## Profile delta

Targeted cost before:
Targeted cost after:

## Interpretation

...

## Decision

KEEP / REJECT / INCONCLUSIVE
```

---

# 11. Experiment scripts

Create reusable scripts rather than ad-hoc shell commands.

Recommended:

```text
experiments/scripts/
├── compare_outputs.py
├── run_correctness.sh
└── run_benchmark.sh
```

## compare_outputs.py

Responsibilities:

```text
load baseline trace
load candidate trace
assert schema equality
assert dtype equality
assert logical row equality

load baseline message ledger
load candidate message ledger
assert schema equality
assert dtype equality
assert logical row equality

print useful first-difference diagnostics
```

Do not merely print:

```text
HASH MISMATCH
```

Report:

```text
first differing row
first differing column
baseline value
candidate value
event/message index
```

---

## run_correctness.sh

Responsibilities:

```text
run baseline fixed-seed AS01
run candidate fixed-seed AS01
compare

run baseline fixed-seed AS06
run candidate fixed-seed AS06
compare

run public regression
```

---

## run_benchmark.sh

Responsibilities:

```text
run control timer
run candidate timer
capture raw results
compute median speedup
compute paired ratios when seeds match
write result artifacts
```

---

# 12. Output directory convention

Never overwrite prior experiment outputs.

Use:

```text
out/
├── control/
│   ├── as01/
│   └── as06/
│
├── E01_scalar_latency_clip/
│   ├── correctness/
│   │   ├── as01/
│   │   └── as06/
│   └── benchmark/
│       ├── as01/
│       └── as06/
│
├── E02_lazy_debug_logging/
│   └── ...
```

If multiple benchmark batches are needed:

```text
benchmark/run01/
benchmark/run02/
benchmark/run03/
```

Never mix outputs from different source revisions.

---

# 13. Build naming convention

Use immutable, descriptive image tags.

Example:

```text
track3-abides-baseline:<commit>
track3-candidate-control:<commit>
track3-e01-scalar-latency:<commit>
track3-e02-lazy-logging:<commit>
```

Do not rely only on:

```text
latest
```

for recorded experiment results.

Record image digest when possible.

---

# 14. Agent execution rules

When an AI coding agent executes an experiment:

1. Read this document first.
2. Confirm current branch.
3. Confirm the branch parent is `exp/t3-candidate-control`.
4. Do not modify `baselines/`.
5. Make only the change described by the experiment.
6. Do not opportunistically clean up unrelated code.
7. Add targeted tests before benchmarking.
8. Run correctness gates before throughput measurement.
9. Preserve all raw benchmark outputs.
10. Write the experiment result record.
11. Do not merge automatically into `t3/integration`.
12. Mark the branch `KEEP`, `REJECT`, or `INCONCLUSIVE`.
13. If correctness fails, stop performance work and diagnose.
14. If performance is within measurement noise, rerun instead of claiming a win.
15. Reprofile only after a meaningful code change or when a result contradicts the hypothesis.

---

# 15. Initial execution order

Recommended order:

```text
E00 candidate control
    ↓
E01 scalar latency clipping
    ↓
E02 lazy debug logging
    ↓
E03 remove dropout analysis
    ↓
E05 copy elision
    ↓
E06/E07 trace construction
    ↓
E08 message trace construction
    ↓
reprofile
    ↓
E11+ structural matching/scheduling experiments
```

If E03 fails correctness:

```text
E03 REJECT
    ↓
E04 vectorize dropout analysis
```

---

# 16. Definition of done

An optimization experiment is complete only when all of the following exist:

```text
[ ] isolated Git branch
[ ] single clearly stated hypothesis
[ ] code change
[ ] targeted correctness tests
[ ] AS01 differential result
[ ] AS06 differential result
[ ] full public regression result
[ ] unprofiled throughput measurement
[ ] raw benchmark artifacts
[ ] experiment result markdown
[ ] KEEP / REJECT / INCONCLUSIVE decision
```

A merged optimization is complete only when it has additionally been:

```text
[ ] cherry-picked into t3/integration
[ ] revalidated in combination
[ ] rebenchmarked cumulatively
```

---

## Guiding principle

The objective is not to accumulate clever code.

The objective is to build a chain of experimentally proven transformations:

```text
baseline
    -> same simulator, faster
    -> same simulator, faster
    -> same simulator, faster
```

Every optimization must have a causal performance claim that can be defended independently.
