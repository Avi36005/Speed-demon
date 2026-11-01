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

## D1 rejected standalone — degree-check experiment

The zero-out-source and zero-in-target tests are mathematically sound, with
s == t checked first. Nevertheless three alternating full public runs were
slower: baseline mean 6.518099 s, candidate 7.632523 s, local wait4 RSS
207,060,992 -> 213,991,424 B. All six hashes pass. These are means of all
three exploratory samples, not the five-run estimator. The extra indegree
array did not pay for itself in this local test; its underlying cause needs
phase profiling. Source reverted to the verified baseline, preserved candidate
`.work/compare/d-d3619d9e6a/candidate/`. A later bidirectional version may
obtain indegrees from its required reverse CSR instead of a separate array.

## Intermediate Linux/runtime proof — 2026-11-01

Current source `0b096c6c3884f90804850fd41461da0cf784c2325fe558a1a400ec5352ec20ca`. Clean offline/no-cache amd64 build
passed in 5.725 s; image 30,364,681 B. One full public
run passed under uid65534, read-only root/data, no network, 8GB/no swap,
PID/scratch restrictions, exact output/hash and no unexpected filesystem diff.
Runtime libraries resolve. This is ARM-host amd64 emulation; no official
T/M/K is inferred. Raw `containers.jsonl`, artifact `.work/containers/d-19907ed6cd`.
Unchanged official static tools now measure S=146, D=1
(declared/unverified dependency basis). Final candidate repeated sandbox and
fresh-data release checks remain separate from this intermediate checkpoint.
