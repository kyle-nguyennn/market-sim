# FastOrderBook implementation plan

The scaffold is complete when it builds and exposes guarded interfaces. The order-book milestone
is complete only when command-by-command differential replay and public semantic gates pass.

## Run the scaffold locally

The commands below validate the current skeleton; they do not run a Track 3 scenario yet. The
native adapter is deliberately not connected to `simulate.py`, and order mutations deliberately
raise until their pinned-ABIDES behavior is captured.

### 1. Check out the implementation branch

From an existing `market-sim` checkout:

```bash
git fetch origin
git switch feat/t3-fast-orderbook
git pull --ff-only
cd agenthon/track3-simulation-public/candidate/native/t3_engine
```

For a new checkout:

```bash
git clone https://github.com/kyle-nguyennn/market-sim.git
cd market-sim
git switch feat/t3-fast-orderbook
cd agenthon/track3-simulation-public/candidate/native/t3_engine
```

All remaining commands assume the current directory is `candidate/native/t3_engine`.

### 2. Install system prerequisites

Ubuntu/Debian or WSL2:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build python3 python3-dev python3-venv
```

macOS with Homebrew:

```bash
xcode-select --install  # Skip if the command-line tools are already installed.
brew install cmake ninja python
```

Required versions:

```text
C++ compiler with C++20 support
CMake >= 3.20
Python >= 3.11
```

### 3. Run the native-only smoke test

This path does not require pybind11 and is the quickest way to validate the C++ structure:

```bash
cmake -S . -B build/native -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DT3_ENGINE_BUILD_PYTHON=OFF \
  -DT3_ENGINE_BUILD_TESTS=ON
cmake --build build/native --parallel
ctest --test-dir build/native --output-on-failure
```

Expected result:

```text
100% tests passed, 0 tests failed out of 1
```

The smoke test expects `add_limit_order()` to throw. That is a guard, not a failure: matching is
still a P1 task.

### 4. Build the Python extension and run binding tests

Create an isolated environment and let scikit-build-core invoke CMake/pybind11:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -e ".[test]"
python -m pytest -q tests/python
```

Expected result:

```text
2 passed
```

Confirm the installed extension manually:

```bash
python - <<'PY'
from t3_fast_orderbook import OrderBook

book = OrderBook()
assert book.best_bid() is None
assert book.best_ask() is None
assert book.active_order_count == 0
print("t3_fast_orderbook import and empty-book checks passed")
PY
```

Use `deactivate` when finished with the virtual environment.

### 5. Optional: build C++ tests and pybind11 together with CMake

Activate the virtual environment from step 4 first, then run:

```bash
pybind11_dir="$(python -m pybind11 --cmakedir)"
cmake -S . -B build/full -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DT3_ENGINE_BUILD_PYTHON=ON \
  -DT3_ENGINE_BUILD_TESTS=ON \
  -Dpybind11_DIR="$pybind11_dir"
cmake --build build/full --parallel
ctest --test-dir build/full --output-on-failure
```

The editable install in step 4 is still the supported way to install/import the extension. This
direct build is useful for compiler diagnostics and native test iteration.

### 6. Optional: run AddressSanitizer and UndefinedBehaviorSanitizer

Use GCC or Clang on Linux/macOS:

```bash
cmake -S . -B build/sanitize -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DT3_ENGINE_BUILD_PYTHON=OFF \
  -DT3_ENGINE_BUILD_TESTS=ON \
  -DT3_ENGINE_ENABLE_SANITIZERS=ON
cmake --build build/sanitize --parallel
ctest --test-dir build/sanitize --output-on-failure
```

### Common failures

- `cmake: command not found` or `ninja: command not found`: repeat the system prerequisite step.
- `Could not find pybind11`: activate `.venv`, run `python -m pip install -e ".[test]"`, or pass
  `-Dpybind11_DIR="$(python -m pybind11 --cmakedir)"` to direct CMake builds.
- Python imports a stale extension: confirm `which python`, activate `.venv`, and reinstall with
  `python -m pip install -e ".[test]"`.
- Compiler rejects C++20: update GCC/Clang or use WSL2 with a current Ubuntu release.
- Do not debug Track 3 trace differences yet: the scaffold is not wired into the simulator until
  the P0 contract and recorder work below are complete.

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
