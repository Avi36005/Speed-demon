# Track D performance evidence

Initial hypothesis: a compact CSR graph and exact forward binary-heap Dijkstra
provide a recoverable correct baseline before advanced routing experiments.
Parsing, construction, every query and output are timed together.

Baseline verification and local measurements are pending. Official live-board
registration/run IDs and all official D measurements remain unknown.

## D0 baseline — 2026-11-01

Source SHA-256 `0b096c6c3884f90804850fd41461da0cf784c2325fe558a1a400ec5352ec20ca`. ARM64 macOS, Apple Clang 17,
C++20 O3/NDEBUG/pthread. Worked and all official hand-written cases, empty
edges, first-meeting trap, 40 random components checked with Floyd–Warshall,
small ASan/UBSan fixtures and independent sample seeds 2/42 all pass.
Public full seed 1 passes the published output hash. Five spawn-to-exit samples:
8.514258, 8.761928, 9.493890, 9.794380, 9.005556 s.
Middle-three mean 9.087125 s; largest wait4 process RSS 207,060,992 B.
These are local ARM process measurements, not scoring-box T/cgroup M. Raw
records: `../benchmarks/track-d/experiments.jsonl` and `suites.jsonl`.
D has not been registered or run on the official board.

Next: measure sound zero-degree rejection, then distance-array reuse and
bidirectional search. Preserve this correct scalar baseline before tuning.
