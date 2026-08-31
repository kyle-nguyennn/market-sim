# Agenthon 2026 — T3 + T2 Execution Plan

**Primary track:** T3 — Semantic-Preserving Market Simulation  
**Secondary track:** T2 — Reasoning-Augmented Time-Series Forecasting  
**Working window:** 31 Aug 2026 → 28 Sep 2026  
**Default allocation:** **70% T3 / 30% T2**

---

## 0. Objective

The goal is not merely to submit to two tracks.

The goal is to leave the competition with two strong, reusable artifacts:

### T3 artifact
A high-performance discrete-event market simulator that preserves:
- matching-engine semantics,
- message causality,
- event ordering,
- stochastic market behavior,
- stylized market facts,

while materially improving throughput over the ABIDES baseline.

### T2 artifact
A probabilistic financial forecasting system that combines:
- multivariate time-series models,
- text-derived macro / regime information,
- calibrated uncertainty,
- joint dependence and tail modeling,

and demonstrates measurable information uplift over text-blind forecasting baselines.

---

# 1. Strategic Priority

## T3 — Primary

Treat T3 as the main competition.

Time allocation:

```text
T3  ███████████████████████████████████  70%
T2  ███████████████                      30%
```

T3 has priority whenever:
- a correctness regression is unresolved,
- a major performance bottleneck is not understood,
- a promising optimization branch needs validation,
- final submission robustness is at risk.

## T2 — Secondary

T2 should remain methodical and bounded.

Do not let it turn into open-ended model experimentation.

The research hierarchy is:

```text
strong numeric baseline
        ↓
calibrated probabilistic forecast
        ↓
text-derived latent regime
        ↓
controlled forecast adjustment
        ↓
ablation
        ↓
retain only measurable uplift
```

---

# 2. Competition Constraints to Keep Visible

## T3

### Core objective

\[
\max \text{events/sec}
\]

subject to:

\[
\text{semantic correctness} = \text{PASS}
\]

\[
\text{stylized-fact checks} = \text{PASS}
\]

### Required output surface

Single scenario:

```text
simulate --config /input/scenario.json --out /output/trace.parquet
```

Batch mode:

```text
simulate-batch --batch-dir /input/scenarios --out-dir /output
```

Important outputs:

```text
trace.parquet
message_trace.parquet
events.json
batch_events.json
```

### Non-negotiable mindset

> Correctness is a hard constraint. Speed is the objective.

Never keep an optimization that improves throughput but breaks semantic equivalence.

---

## T2

### Core objective

Produce Monte Carlo draws approximating:

\[
p(Y_{t+h}\mid X_{\le t}, T_{\le t})
\]

where:
- \(X\) = numeric financial panel,
- \(T\) = frozen text corpus.

### Score

\[
S =
0.5 \cdot \text{CRPS}
+
0.3 \cdot \text{joint variogram}
+
0.2 \cdot \text{tail penalty}
\]

Lower is better.

### Scientific question

Does text improve the forecast?

Always maintain two versions:

```text
A. numeric only
B. numeric + text
```

Measure:

\[
\Delta S = S_B - S_A
\]

Desired:

\[
\Delta S < 0
\]

---

# 3. Working Rules

## Rule 1 — Baseline before optimization

For both tracks:

```text
reproduce
→ validate
→ profile
→ improve
```

Do not modify architecture before reproducing the official baseline.

---

## Rule 2 — One experiment, one hypothesis

Every meaningful experiment must state:

```text
Hypothesis:
Change:
Metric:
Expected result:
Observed result:
Decision:
```

Example:

```text
Hypothesis:
Python object allocation dominates T3 throughput.

Change:
Replace per-event message objects with packed arrays.

Metric:
Median events/sec across representative public scenarios.

Expected:
+20% throughput.

Observed:
+7%, but one causality regression.

Decision:
Reject current implementation.
```

---

## Rule 3 — Preserve known-good checkpoints

Use branches or tags:

```text
baseline
correctness-clean
perf-1
perf-2
t2-numeric-baseline
t2-text-v1
final-candidate
```

Never optimize on top of an unverified state.

---

## Rule 4 — Measure on fixed benchmark subsets

Do not compare performance across arbitrary scenarios.

Create:

### T3 development benchmark set

```text
T3_FAST
T3_MEDIUM
T3_HEAVY
T3_PROTOCOL
T3_STOCHASTIC
T3_BATCH
```

Pick representative scenarios from the public suite.

Track:
- events/sec,
- wall time,
- peak memory,
- output equality,
- gate status.

### T2 development benchmark set

Pick representative cards across:
- rates,
- FX,
- macro,
- factors,
- short horizon,
- long horizon,
- single asset,
- multi-asset.

Track:
- marginal CRPS,
- variogram score,
- tail penalty,
- normalized composite,
- calibration,
- runtime.

---

# 4. Week 1 — Establish Ground Truth

## Dates
**31 Aug – 6 Sep**

## Main objective

By the end of Week 1:

### T3
You can run the official ABIDES baseline, reproduce public outputs, run regression checks, and explain where runtime goes.

### T2
You can run the official forecasting pipeline, produce valid Monte Carlo forecasts, and compare multiple text-blind baselines.

---

# Day 1 — Environment + Baselines

## T3

- [x] Clone official T3 repository.
- [ ] Read:
  - [x] README
  - [ ] concepts documentation
  - [x] baseline README
  - [x] scoring / regression documentation
- [x] Build baseline ABIDES Docker image.
- [x] Run one simple public scenario.
- [ ] Confirm generation of:
  - [ ] `trace.parquet`
  - [ ] `message_trace.parquet`
  - [ ] `events.json`
- [ ] Run local scorer / regression check.
- [x] Record baseline throughput.

### Deliverable

```text
notes/t3_baseline.md
```

Must contain:
- exact setup,
- exact command,
- scenario ID,
- pass/fail,
- events,
- wall time,
- events/sec.

---

## T2

- [ ] Clone official T2 repository.
- [ ] Read:
  - [ ] README
  - [ ] scoring definition
  - [ ] baseline documentation
  - [ ] submission CLI
- [ ] Run reference forecast CLI.
- [ ] Produce:
  - [ ] `forecast.parquet`
  - [ ] `forecast_meta.json`
  - [ ] rationale output if applicable
- [ ] Pass admissibility checks.
- [ ] Run at least one text-blind baseline.

### Deliverable

```text
notes/t2_baseline.md
```

---

# Day 2 — Understand the T3 Semantic Contract

Read the simulator as a system.

Map:

```text
scenario
   ↓
config builder
   ↓
kernel
   ↓
event queue
   ↓
agents
   ↓
messages
   ↓
exchange agent
   ↓
order book
   ↓
event trace
```

For each component, write:

```text
state
inputs
outputs
invariants
hot-path likelihood
```

## Specific components to inspect

- [ ] Kernel / event scheduler
- [ ] Message transport
- [ ] Latency model
- [ ] Agent wakeup
- [ ] ExchangeAgent
- [ ] Limit-order book
- [ ] Order submission
- [ ] Cancel
- [ ] Replace
- [ ] Partial fill
- [ ] Full fill
- [ ] Trace generation
- [ ] Parquet serialization

### Deliverable

```text
notes/t3_architecture_map.md
```

---

# Day 3 — T3 Differential Harness

Build a comparison tool:

```text
reference output
        vs
candidate output
```

It should classify mismatch types.

## Required comparisons

- [ ] row count
- [ ] event type
- [ ] price
- [ ] size
- [ ] order ID
- [ ] fill order
- [ ] timestamps
- [ ] agent ID
- [ ] message ledger
- [ ] first divergence location

### Desired output

```text
PASS

or

FAIL
first mismatch: event 38192
field: order_id
reference: 1298
candidate: 1301
previous 10 events: ...
next 10 events: ...
```

### Deliverable

```text
tools/trace_diff.py
```

This is one of the highest-value tools in the entire project.

---

# Day 4 — Profile T3

Profile multiple scenarios.

Break total runtime into:

\[
T =
T_{scheduler}
+
T_{agent}
+
T_{exchange}
+
T_{LOB}
+
T_{trace}
+
T_{serialization}
+
T_{other}
\]

Use:
- cProfile,
- py-spy,
- line profiler if helpful,
- allocation profiling if useful.

## Questions to answer

- [ ] How many Python function calls occur per event?
- [ ] How many objects are allocated per event?
- [ ] Is event-queue maintenance significant?
- [ ] Is matching significant?
- [ ] Is trace collection significant?
- [ ] Is Parquet writing significant?
- [ ] Which agent types dominate runtime?
- [ ] Does cost change materially with scenario family?

### Deliverable

```text
notes/t3_profile_v1.md
```

Rank bottlenecks:

```text
1.
2.
3.
4.
5.
```

---

# Day 5 — T2 Numeric Baseline Matrix

Run several text-blind models.

Minimum:

- [ ] simple persistence / naïve benchmark
- [ ] Theta / AutoARIMA
- [ ] one foundation model
- [ ] second foundation model if operationally reasonable
- [ ] ensemble

For each card type, ask:

```text
Which model is strongest?
Which model is best calibrated?
Which model handles tails best?
Which model captures dependence best?
```

### Build one baseline ensemble

Example:

\[
F =
w_1F_{\text{ARIMA}}
+
w_2F_{\text{Chronos}}
+
w_3F_{\text{TimesFM}}
\]

Weights must be selected systematically.

Start with:

```text
equal weights
```

then test simple validation-based weighting.

### Deliverable

```text
notes/t2_numeric_baselines.md
```

---

# Day 6 — T2 Probabilistic Calibration

Do not work on text yet.

Inspect:
- [ ] forecast dispersion
- [ ] empirical coverage
- [ ] tail behavior
- [ ] cross-asset dependence
- [ ] horizon dependence

Potential adjustments:

### Variance scaling

\[
\tilde{Y}
=
\mu + c(Y-\mu)
\]

Search \(c\).

### Heavy-tail transformation

Consider:
- Student-t residuals,
- empirical residual bootstrap,
- quantile mapping.

### Dependence

For multi-asset cards, preserve joint structure.

Potential baseline:

```text
marginal forecast
+
historical residual correlation
+
copula / residual resampling
```

### Deliverable

```text
src/t2/calibration/
```

---

# Day 7 — Weekly Review

## T3 checkpoint

You must know:

- [ ] baseline passes public regression
- [ ] benchmark subset defined
- [ ] top bottlenecks identified
- [ ] differential harness works
- [ ] no semantic ambiguity blocking optimization

## T2 checkpoint

You must have:

- [ ] admissible forecast pipeline
- [ ] reproducible baseline
- [ ] numeric-only ensemble
- [ ] baseline calibration report

## Week 1 decision

Write:

```text
What do I now believe is the highest-value T3 optimization?

What is the strongest T2 numeric baseline?

What is still unknown?
```

---

# 5. Week 2 — First Real Improvements

## Dates
**7 Sep – 13 Sep**

---

# T3 Goal

Achieve the first material throughput gain while preserving all public semantics.

Target:

\[
+20\% \text{ median throughput}
\]

A smaller improvement is acceptable if it reveals the dominant architecture.

---

## T3 Optimization Order

Do not jump immediately to C++ or Rust.

Try optimizations in this order.

### Level 1 — remove obvious overhead

Investigate:

- [ ] trace buffering
- [ ] avoid repeated DataFrame construction
- [ ] reduce logging
- [ ] reduce object copying
- [ ] cache immutable lookups
- [ ] reduce dictionary lookup in hot loops
- [ ] avoid repeated attribute resolution
- [ ] batch output conversion
- [ ] preallocate arrays where possible

After every change:

```text
benchmark
+
full relevant regression
```

---

### Level 2 — data structures

Inspect:

#### Event queue
Current likely pattern:

\[
O(\log N)
\]

with heap operations.

Ask:
- Is queue size large?
- Is event time monotone?
- Are timestamps sufficiently structured?
- Can bucket queues / calendar queues help?

Do not replace the heap unless profiling justifies it.

#### Order book

Inspect:
- price-level lookup,
- best bid / ask,
- FIFO queue,
- cancel lookup,
- replace lookup.

Desired complexity:

```text
best price     O(1) / amortized O(1)
order lookup   O(1)
cancel         O(1)
FIFO           O(1)
```

Where practical.

---

### Level 3 — object model

Measure cost of:

```text
Event
Message
Order
Agent
```

Potential replacement:

```text
struct-of-arrays
packed dataclass
slots
NumPy arrays
typed memory
```

---

# T2 Goal

Add a text component that produces a **controlled forecast adjustment**.

Do not ask an LLM to directly predict prices.

---

## T2 Text Architecture v1

### Step 1 — retrieve relevant documents

Filter by:
- as-of date,
- panel,
- asset,
- macro relevance,
- recency.

### Step 2 — infer latent state

For example:

```json
{
  "growth": -0.4,
  "inflation": 0.7,
  "policy_hawkishness": 0.8,
  "risk_sentiment": -0.3,
  "uncertainty": 0.6
}
```

### Step 3 — transform latent state into forecast adjustment

General form:

\[
\mu' = \mu + \beta^\top z
\]

\[
\sigma' = \sigma \cdot e^{\gamma^\top z}
\]

Optional dependence adjustment:

\[
\Sigma' = f(\Sigma, z)
\]

### Step 4 — generate Monte Carlo paths

Text modifies the numeric forecast.

It does not replace it.

---

## T2 Initial Regime Vocabulary

Start small.

### Rates
- hawkish
- dovish
- inflation upside
- growth downside
- uncertainty spike

### FX
- relative-rate differential shift
- risk-on
- risk-off
- intervention concern

### Factors
- momentum regime
- value regime
- quality / defensive regime
- liquidity stress

Avoid hundreds of labels.

---

# Week 2 Experiments

## T3

- [ ] optimization experiment #1
- [ ] optimization experiment #2
- [ ] optimization experiment #3
- [ ] preserve best branch only

## T2

- [ ] text state extraction v1
- [ ] deterministic structured output
- [ ] simple linear adjustment
- [ ] numeric-only vs numeric+text ablation

---

# Week 2 Gate

## T3

Proceed to low-level rewrite only if:

```text
remaining dominant cost is clearly inside a hot loop
AND
Python-level optimization is plateauing
```

## T2

Continue text modeling only if:

```text
text shows improvement on a non-trivial subset
OR
there is a clear calibration failure that text plausibly addresses
```

Otherwise simplify.

---

# 6. Week 3 — Specialization

## Dates
**14 Sep – 20 Sep**

This is the highest-leverage week.

---

# T3 — Compiled Core Decision

Choose among:

```text
A. optimized Python
B. Numba
C. Cython
D. C++
E. Rust / PyO3
```

Decision must be based on:

```text
profile
implementation cost
FFI overhead
determinism risk
semantic risk
expected speedup
```

---

## Preferred architecture if compiled rewrite is justified

```text
Python
│
├── scenario parser
├── config validation
├── CLI
└── output orchestration
        │
        ▼
Compiled simulator
│
├── event scheduler
├── agent state
├── latency handling
├── matching engine
├── order state
└── trace buffer
        │
        ▼
Arrow-compatible memory
        │
        ▼
Parquet
```

Critical rule:

> Do not cross Python ↔ native boundary per event.

Use one coarse call:

```text
simulate(config)
```

not millions of FFI calls.

---

# T3 Data-Oriented Design

Evaluate replacing object-heavy structures with arrays.

Potential representation:

```text
orders:
    id[]
    owner[]
    side[]
    price[]
    qty[]
    status[]
    next[]
    prev[]

events:
    time[]
    seq[]
    src[]
    dst[]
    type[]
    payload[]
```

Keep:
- deterministic sequence IDs,
- explicit tie-breaking,
- stable order IDs.

---

# T3 Correctness Attack Tests

Create adversarial scenarios for:

- [ ] equal timestamps
- [ ] same-price FIFO
- [ ] partial fill
- [ ] cancel after partial fill
- [ ] replace
- [ ] cancel/replace race
- [ ] simultaneous wakeups
- [ ] zero latency
- [ ] heterogeneous latency
- [ ] multiple agents hitting same level
- [ ] empty book
- [ ] book transition through zero depth

These should become permanent regression tests.

---

# T2 — Forecast Architecture v2

By now the pipeline should resemble:

```text
                  ┌────────────────┐
numeric panel ───▶│ base forecaster│
                  └───────┬────────┘
                          │
                          ▼
                     distribution
                          │
                          │
text corpus ───▶ retrieval
                     │
                     ▼
               regime inference
                     │
                     ▼
                adjustment layer
                     │
                     ▼
               calibrated samples
```

---

# T2 Better Adjustment Models

Try in order:

### 1. Hand-designed mappings

Example:

```text
hawkish:
    rates mean ↑
    rates variance ↑

risk-off:
    JPY strength
    high-beta FX weakness
    factor dispersion ↑
```

Good for interpretability.

---

### 2. Linear learned mapping

\[
\Delta \mu = Bz
\]

\[
\log \sigma' = \log \sigma + Cz
\]

Fit only on historical windows respecting cutoff.

---

### 3. Small nonlinear model

Only if justified:

```text
MLP
gradient boosting
small Bayesian model
```

Avoid unnecessary complexity.

---

# T2 Ablation Matrix

Every model must be compared across:

| Variant | Numeric | Text | Calibration | Dependence |
|---|---|---|---|---|
| A | ✓ | | | |
| B | ✓ | | ✓ | |
| C | ✓ | | ✓ | ✓ |
| D | ✓ | ✓ | ✓ | ✓ |

The goal is to isolate where gains come from.

---

# 7. Week 4 — Competition Mode

## Dates
**21 Sep – 28 Sep**

No uncontrolled rewrites after the middle of this week.

---

# T3 Finalization

## 21–23 Sep

Focus:

- [ ] full public regression suite
- [ ] batch scenarios
- [ ] message ledger
- [ ] determinism
- [ ] peak memory
- [ ] cold-start behavior
- [ ] repeated throughput runs

Measure:

```text
median
p25
p75
worst-case
```

Do not report only best run.

---

## 24 Sep

Freeze:

```text
T3_RELEASE_CANDIDATE_1
```

Only accept changes that:
- fix correctness,
- materially improve robustness,
- provide clearly demonstrated speedup.

---

## 25–26 Sep

Attack the candidate.

Run:
- clean machine build,
- fresh Docker build,
- no cached state,
- repeated scenario runs,
- randomized scenario order,
- full validation.

Check for:
- nondeterminism,
- hidden filesystem assumptions,
- relative paths,
- environment dependencies,
- stale generated files.

---

## 27 Sep

Freeze final candidate.

Produce:

```text
T3_FINAL
```

Record:
- image hash,
- git commit,
- benchmark numbers,
- regression result,
- environment.

---

## 28 Sep

Submission-only day.

No architectural work.

---

# T2 Finalization

## 21–23 Sep

Run systematic comparison.

For every meaningful card family:

```text
numeric baseline
vs
text model
vs
calibrated text model
```

Ask:

- Does text improve mean score?
- Does text improve only certain assets?
- Does text improve tails but hurt mean?
- Does it hurt joint dependence?
- Does it overreact to stale text?

---

## T2 Decision Rule

If text is harmful in a family, disable or downweight it there.

Use a router:

```text
if evidence quality low:
    numeric forecast only
else:
    numeric + text adjustment
```

Potential confidence weighting:

\[
\mu' = \mu + q \Delta\mu
\]

where:

\[
q \in [0,1]
\]

is confidence in the extracted regime.

---

## 24–25 Sep

Tune only:
- ensemble weights,
- variance scaling,
- text confidence,
- tail scale,
- dependence blend.

Do not introduce new model families.

---

## 26 Sep

Freeze:

```text
T2_RELEASE_CANDIDATE_1
```

---

## 27 Sep

Full dry run:
- all required files,
- Docker entrypoint,
- offline behavior,
- model endpoint behavior,
- timeout,
- deterministic metadata.

---

## 28 Sep

Final submission.

---

# 8. Daily Operating Rhythm

Recommended daily structure:

## Block A — T3 Deep Work
**2.5–4 hours**

One major goal only.

Examples:

```text
profile event queue
rewrite trace buffer
optimize order lookup
validate compiled core
```

---

## Block B — T2 Experiment
**1–2 hours**

One controlled experiment.

Examples:

```text
calibrate variance
test Chronos ensemble
extract hawkishness state
measure text uplift
```

---

## Block C — Review
**20–30 minutes**

Write:

```text
What changed?
What did I measure?
What did I learn?
What branch is currently best?
What is tomorrow's single highest-value experiment?
```

---

# 9. T3 Experiment Log Template

Copy for every experiment.

```text
Experiment ID:
Date:

Hypothesis:

Baseline commit:
Candidate commit:

Scenarios:

Change:

Expected effect:

Correctness:
[ ] PASS
[ ] FAIL

Baseline events/sec:
Candidate events/sec:

Delta:
_____ %

Memory delta:

Observed bottleneck shift:

Unexpected behavior:

Decision:
[ ] keep
[ ] revert
[ ] investigate further

Next experiment:
```

---

# 10. T2 Experiment Log Template

```text
Experiment ID:
Date:

Hypothesis:

Dataset / card families:

Numeric baseline:

Text method:

Adjustment method:

Calibration:

Baseline score:
Candidate score:

CRPS delta:
Variogram delta:
Tail delta:
Composite delta:

Improved families:

Harmed families:

Interpretation:

Decision:
[ ] keep
[ ] revert
[ ] conditional use
[ ] investigate further
```

---

# 11. T3 Performance Investigation Checklist

Before doing a native rewrite, answer all of these.

## Scheduler

- [ ] queue length distribution measured
- [ ] pushes/event measured
- [ ] pops/event measured
- [ ] heap runtime percentage known
- [ ] tie-breaking semantics understood

## Agents

- [ ] dominant agent types known
- [ ] wakeup frequency measured
- [ ] message frequency measured
- [ ] avoidable callbacks identified

## Matching engine

- [ ] order lookup complexity known
- [ ] price-level lookup complexity known
- [ ] best-price retrieval complexity known
- [ ] cancel complexity known
- [ ] fill complexity known

## Memory

- [ ] major allocations/event identified
- [ ] object lifetime understood
- [ ] garbage collection contribution measured

## Trace

- [ ] trace construction cost measured
- [ ] message-ledger cost measured
- [ ] Parquet cost measured
- [ ] opportunity for batch serialization measured

Only then decide on architecture.

---

# 12. T2 Research Checklist

## Forecast distribution

- [ ] correct forecast horizon
- [ ] sufficient Monte Carlo draws
- [ ] no leakage
- [ ] calibrated scale
- [ ] plausible tails
- [ ] joint dependence preserved

## Text

- [ ] cutoff respected
- [ ] retrieval deterministic
- [ ] recency captured
- [ ] relevance captured
- [ ] contradictory documents handled
- [ ] uncertainty represented

## Evaluation

- [ ] numeric-only benchmark preserved
- [ ] all gains ablated
- [ ] no single-card overfitting
- [ ] family-level behavior inspected
- [ ] worst-case regressions understood

---

# 13. Kill Criteria

These prevent wasting September.

## Kill / defer a T3 idea if

- correctness breaks repeatedly,
- speedup < 5% after substantial complexity,
- implementation requires rewriting unrelated components,
- regression behavior becomes opaque,
- hidden-scenario generalization likely decreases.

---

## Kill / defer a T2 idea if

- improvement only appears on one or two cards,
- text effect cannot be explained,
- result depends heavily on prompt randomness,
- text improves mean but destroys tails,
- text improves marginals but destroys dependence,
- model complexity is high relative to gain.

---

# 14. Stretch Goals

Only attempt after core submission is safe.

## T3

- [ ] batch-aware execution
- [ ] specialized monotone event queue
- [ ] arena allocator
- [ ] lock-free / parallel independent simulation
- [ ] vectorized agent updates
- [ ] compiled Arrow output
- [ ] SIMD-friendly LOB state
- [ ] automated profile comparison

## T2

- [ ] regime-conditioned covariance
- [ ] mixture distributions
- [ ] Bayesian model averaging
- [ ] conformal calibration
- [ ] dynamic model weighting
- [ ] text-confidence gating
- [ ] learned cross-asset propagation of macro state

---

# 15. Final Submission Checklist — T3

## Build

- [ ] clean Docker build
- [ ] no undeclared dependency
- [ ] correct CLI
- [ ] network disabled
- [ ] fresh-machine reproducibility

## Correctness

- [ ] all public Tier-A checks
- [ ] all public Tier-B checks
- [ ] stylized-fact gates
- [ ] message ledger
- [ ] batch isolation
- [ ] deterministic repeated outputs

## Performance

- [ ] benchmark repeated
- [ ] warmup handled
- [ ] median recorded
- [ ] memory checked
- [ ] no benchmark-specific hardcoding

## Reproducibility

- [ ] git commit recorded
- [ ] image hash recorded
- [ ] config recorded
- [ ] benchmark script committed

---

# 16. Final Submission Checklist — T2

## Build

- [ ] Docker builds cleanly
- [ ] `forecast` CLI works
- [ ] correct paths
- [ ] restricted-network behavior tested
- [ ] model endpoint configuration tested

## Output

- [ ] Monte Carlo draws valid
- [ ] schema valid
- [ ] metadata valid
- [ ] all required horizons present
- [ ] all required assets present

## Modeling

- [ ] numeric baseline stable
- [ ] calibration stable
- [ ] text uplift measured
- [ ] confidence gating enabled if useful
- [ ] tails checked
- [ ] dependence checked

## Reproducibility

- [ ] model version pinned
- [ ] seed pinned
- [ ] prompt version pinned
- [ ] training / data cutoff documented
- [ ] git commit recorded
- [ ] image hash recorded

---

# 17. Success Criteria

## Minimum acceptable outcome

### T3
- valid final submission,
- all public semantic gates pass,
- measurable throughput improvement over baseline.

### T2
- valid final submission,
- strong numerical forecast,
- calibrated probabilistic output,
- at least a defensible text-ablation result.

---

## Strong outcome

### T3

\[
\boxed{
2\times+
\text{ baseline throughput}
}
\]

with clean semantic regression.

### T2

Consistent:

\[
\boxed{
S_{\text{text+numeric}}
<
S_{\text{numeric}}
}
\]

across multiple families.

---

## Exceptional outcome

### T3
A compiled, data-oriented engine with a large speedup and strong generalization to sealed scenarios.

### T2
A robust regime-conditioned probabilistic forecasting system where the contribution of text is measurable, interpretable, and stable.

---

# 18. What Not to Do

## T3

Do not:
- rewrite everything immediately,
- optimize without profiling,
- trust one scenario,
- accept tiny gains with large semantic risk,
- parallelize before understanding determinism,
- benchmark only the fastest run.

## T2

Do not:
- ask an LLM to guess tomorrow's price directly,
- treat rationale quality as forecast quality,
- optimize only marginal error,
- ignore tails,
- ignore dependence,
- use post-cutoff data,
- confuse leaderboard tuning with genuine uplift.

---

# 19. One-Sentence Mental Models

## T3

> Reimplement the same market more efficiently.

## T2

> Start with a good probability distribution, then use text only when it gives additional information.

---

# 20. First Immediate Actions

## T3

- [ ] baseline Docker build
- [ ] run one public scenario
- [ ] verify output
- [ ] run regression
- [ ] inspect Kernel
- [ ] inspect ExchangeAgent
- [ ] inspect OrderBook
- [ ] create benchmark sheet

## T2

- [ ] run reference CLI
- [ ] run Theta/ARIMA baseline
- [ ] run one foundation model baseline
- [ ] inspect output draws
- [ ] inspect score components
- [ ] create numeric-only benchmark sheet

---

# 21. End-of-Day Questions

Print this section and answer it every night.

```text
1. What did I prove today?

2. What did I merely assume?

3. What is the current best T3 commit?

4. What is the current best T2 model?

5. What metric changed?

6. Did the change generalize?

7. What failed?

8. What did that failure reveal?

9. What is the single highest-value experiment tomorrow?

10. Am I optimizing the actual competition metric?
```

---

# Final Principle

The two-track strategy is coherent:

\[
\boxed{
T3 = model the market mechanism
}
\]

\[
\boxed{
T2 = model the market distribution
}
\]

Treat both as empirical engineering problems:

\[
\text{hypothesis}
\rightarrow
\text{implementation}
\rightarrow
\text{measurement}
\rightarrow
\text{verification}
\rightarrow
\text{iteration}.
\]

Do not optimize what you have not measured.  
Do not trust what you have not independently verified.
