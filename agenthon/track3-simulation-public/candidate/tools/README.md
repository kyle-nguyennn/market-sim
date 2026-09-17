# Order-book differential tools

This directory will hold the Python-only tooling used to treat pinned ABIDES as an executable
specification. It is separate from `native/t3_engine` so recorder/replay code cannot leak ABIDES
objects into the C++ core.

Planned tools:

- `record_orderbook_stream.py` — capture primitive commands, ordered results, and compact state
  snapshots without changing RNG calls or event ordering.
- `replay_orderbook_stream.py` — feed the same versioned JSONL stream to an implementation.
- `diff_orderbook.py` — run ABIDES and native books side-by-side and report the first divergence.

TODO: define the versioned fixture schema and prove that observation-only recording leaves the
baseline trace and message ledger byte-for-byte unchanged before committing scenario captures.

