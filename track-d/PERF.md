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

## Official live-board checkpoint — cec7edf

Run `run-de4b3b2aeb82`, ended 2026-11-01T18:21:18Z. All check/build/correctness/
timeout/sandbox gates passed. Public full native scoring-box T=9.731496 s,
cgroup M=115,175,424 B, S=146, D=1, K=3.426991,
machine points=26.745647. Registered in this feed: False.
Raw receipt `../benchmarks/board/2026-11-01T182140.380992Z.json`.
This is one real live public run, not final holdout/judging.

## Official round-three result — 7f739ca

Run `run-8705e4261a3d`, 2026-11-01T18:37:37Z; all gates pass. T=9.149643 s,
cgroup M=115,265,536 B, S=146, D=1, K=3.533725,
points=27.411636. Registered=False. Raw receipt
`../benchmarks/board/2026-11-01T183856.248929Z.json`. Single live public run.

## D3 retained for actual-box trial — exact bidirectional candidate

The candidate combines two-pass forward/reverse CSR, exact bidirectional search,
reused touched labels and independent query workers. Individual contributions
are not separately measured. All-pairs random graphs, first-meeting trap, zero
cycles, parallel/reverse edges, UINT32_MAX chains, isolated/same-node cases and
small four-worker sanitizers pass, as do sample42 and fresh full seed7.
Five alternating public runs: scalar D0 middle-three4.557196 s, candidate two
workers1.018434 s, process RSS207,028,224 ->200,949,760 B. Later identical-source
worker sweeps: two0.774196 ->four0.483856 s (RSS223,444,992 B); independent
two0.767158 ->six0.425847 s (RSS281,526,272 B). Four chosen as a provisional
time/memory compromise; these separate blocks are not a direct four/six
controlled comparison. Raw comparisons.jsonl retains every sample and digest.

Current default-four source `10d03e6f17ee6df7f44e11816f767b8daf8f37e73873c2e9c267e4eb41d1332a`
passed a fresh full7 reference check and five exact repeated runs:
0.498242,0.496833,0.497163,0.497189,0.497724 s; peak wait4 RSS223,232,000 B.
Fresh output SHA256 `66d63e1c9c1f80583914b835d82fd24e1bd78fba3bdc1cf73efb7f60ed4fe646`.
These are ARM64 process measurements. Current-source clean container proof and
real D3 board axes are still pending. Registration remains false in the feed.

## Round-four exact-source proof

Digest `10d03e6f17ee6df7f44e11816f767b8daf8f37e73873c2e9c267e4eb41d1332a`. Five public native runs pass; local middle-three mean
0.484321 s, max wait4 process RSS 223,592,448 B. These
ARM process samples include input mapping and are not scoring-machine axes.
Clean offline amd64 build plus FIVE restricted public runs and independent
full7 all pass, including exact outputs, libraries and filesystem diff checks.
Public container record `.work/containers/d-e845297277`; fresh `.work/containers/d-638e3df206`.
Unchanged official static tools measure S=239, D=1
(declared/unverified). Current native matrix gaps are being filled before
pushing. Actual round-four official run IDs/axes remain pending.

## Official round-four result — 1429e2e

Run `run-48c61f525199`, ended 2026-11-01T19:24:57Z. All five gates pass.
T=1.259096 s, cgroup M=210,563,072 B, S=239, D=1,
K=7.842636, machine points=44.723104. Registered=False.
Raw receipt `../benchmarks/board/2026-11-01T192637.287985Z.json`.
Single public live run; final holdout/judging remain separate.

### D4 radix-frontier experiment — rejected locally

Hypothesis: each directional heap has monotone uint64 keys under nonnegative
weights, so65 radix buckets might reduce binary-heap comparisons. All semantic
fixtures, random all-pairs, ASan/UBSan at12workers and sample42 passed. Five
alternating ARM public-full pairs regressed from .493666 to .553765s; peak
process RSS223,690,752 ->226,557,952B. Keep D3. No x86 benefit or new official
score is claimed. Source/cmds/raw samples retained in comparisons.jsonl.

### Actual board receipt — f95f205

Run `run-848af1be5c83`: T1.229514s, cgroup M210,100,224B, S239, D1, K7.939916, machine points44.990795; all gates passed. Receipt benchmarks/board/2026-11-01T195245.141719Z.json. Only A build inputs changed since1429e2e. A score +.229959 in one run is within observed run variation; PGO benefit remains uncertain. B/C/D changes here are run variation because their runtime inputs are identical. D remains unregistered.

### Actual board trial5c216d1 — all gates passed

Run `run-ed34233348c5`: T1.116187s, cgroup M210,255,872B, S239, D1, K8.332011, machine points46.037492. Immutable receipt: benchmarks/board/2026-11-01T200313.061179Z.json. A13/B11/C9 registered overall57.177641 (geometric mean). D remains unregistered and its runtime bytes unchanged; its time change is variation, not a new algorithm. All A/B/C scored above the previous recorded best; retain these verified candidates.

## 2026-11-02: parallel build and 32-bit distances

| Version | Local T (4 workers) | Board T | Board M | Board points |
|---|---:|---:|---:|---:|
| D3 (`5c216d1`) | 2.5 s | 1.116 s | 210.3 MB | 46.04 |
| Parallel two-pass CSR build, 12 workers (`71f59b0`) | 1.83 s | 0.504 s | 440.0 MB | 51.71 |
| 32-bit distances + exact side map (`a8f69d1`) | 1.75 s | 0.471 s | 272.1 MB | 54.31 |
| Owner-range fill without atomics (`35d1a42`) | 1.46 s | 0.608 s | 271.0 MB | 51.32 |

The owner-range fill was faster locally but slower on the board in two runs
(0.608 s and 0.580 s against 0.471 s), so `e05f716` restores the atomic fill.
Plain unidirectional Dijkstra was measured at 2.4× the bidirectional query time
and was not used. D is still unregistered on the board.

### Registration refresh

D is registered and passing in current2026-11-02 receipts (including ccc259f/
run-aa2034569555). Earlier unregistered statements describe historical snapshots.
No D runtime change during these B/C trials; its timing fluctuations are variation.

## Four-track portfolio trial — native pending

Private sparse distance tables replace two fully initialized node arrays per worker. Full uint64 distances retain the exact side map. Clear only occupied slots, grow before half load, preserve all Dijkstra bounds.
Source055f90aca740e30b7acf3b28092176e8e4f23b6364018b6d4d9c9cded52fd226, S273/D1. Fixtures UBSan/bounds1/12,
fresh full7, five offline exact-image public runs and one fresh-image full7
pass. Five paired QEMU means1.001046 ->
0.873029s; not scoring-machine T/M. Exact
proof ledger benchmarks/four-track-portfolio-trial.json. Native ASan and board
effect pending; preserve prior source and select by complete native axes.
