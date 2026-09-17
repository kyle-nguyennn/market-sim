# T3 native engine scaffold

This directory is the buildable home for the native Track 3 simulator. The first component is a
C++ `FastOrderBook` exposed to Python through pybind11. The mutation methods deliberately fail
fast for now: the scaffold defines ownership and interfaces without claiming ABIDES-equivalent
matching before the differential oracle exists.

## Layout

```text
include/t3/orderbook/       public C++ records and API
src/orderbook/              native implementation
bindings/python.cpp         pybind11 translation only
python/t3_fast_orderbook/   import surface for Python
tests/cpp/                  native smoke tests
tests/python/               binding smoke tests
```

ABIDES-specific translation stays in `candidate/abides_fork/fast_orderbook_adapter.py`. The C++
core must not import, retain, or manipulate Python/ABIDES objects.

## Build

Native-only development build:

```bash
cmake -S . -B build \
  -DT3_ENGINE_BUILD_PYTHON=OFF \
  -DT3_ENGINE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Python editable install:

```bash
python -m pip install -e .
python -m pytest tests/python
```

Sanitizer build:

```bash
cmake -S . -B build-sanitize \
  -DT3_ENGINE_BUILD_PYTHON=OFF \
  -DT3_ENGINE_ENABLE_SANITIZERS=ON
cmake --build build-sanitize
ctest --test-dir build-sanitize --output-on-failure
```

See [ASSUMPTIONS.md](ASSUMPTIONS.md) for the provisional contract and [TODO.md](TODO.md) for the
implementation gates. Any assumption contradicted by the pinned ABIDES baseline must be changed
before matching logic is added.

