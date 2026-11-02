# Track A performance evidence

First baseline: mmap, input-backed endpoint keys, uint32 latency vectors, full
bucket sorting, one worker. Measurements will be recorded by `tools/dev.py` in
`benchmarks/track-a/experiments.jsonl`, including source digest and commands.

At the initial local-baseline checkpoint, official board results were unavailable.
The first real live-board result is recorded in the dated section below.
Final container/runtime validation has passed; local RSS is not cgroup memory.

## 2026-11-01 native baseline

Source SHA-256: `21e0f155b9f5e076bfa80c957948d354d4539fb0ba5d4ed73495640c1e3c772a`.
Apple Clang 17, ARM64 macOS, `-std=c++20 -O3 -DNDEBUG -pthread`.
Public full seed 1 output matched the untouched reference and published hash
`495ca09682b9673ef836a506496177680bb4a3af54255174befddb1f83481cd3`.
Five local spawn-to-exit times: 1.158741, 0.744164, 0.721905, 0.703902,
0.734554 seconds. Mean of middle three: 0.733541 s. Maximum local process
RSS: 286,588,928 bytes, collected with wait4. This is not official T or M.
Docker was starting/preparing images during the session; these are provisional
development measurements, not a controlled comparison with later experiments.

Worked example, official edge fixture, empty input, percentile/case/prefix/long
endpoint fixture, tiny and sample seed 1 all matched. Small edge and targeted
fixtures passed ASan/UBSan. The first benchmark wrapper failed because sandboxed
`time -l` could not read `kern.clockrate`; output correctness had already passed.
That failed experiment is retained, and the runner now uses wait4 directly.

## A1 retained — independent whole-line workers

Each worker owns its endpoint/hour vectors. Cuts advance to a complete line
boundary; merging retains every latency and error before exact full sorting.

| Workers | Paired scalar mean (s) | Candidate mean (s) | Candidate peak RSS (bytes) |
|---|---:|---:|---:|
| 1 | 0.570089 | 0.631456 | 286,441,472 |
| 2 | 0.688375 | 0.493506 | 291,504,128 |
| 4 | 0.442366 | 0.252881 | 301,547,520 |
| 6 | 0.461982 | 0.234380 | 311,377,920 |
| 12 | 0.435864 | 0.229677 | 341,508,096 |

Five alternating runs per version, Apple Clang 17, ARM64, identical full seed 1.
All fifty outputs matched. Baselines drift across blocks; compare each candidate
to its own paired baseline. Six workers selected: twelve had no reliable time
advantage and used more RSS. No x86 worker-count advantage is inferred.
The one-worker parallel scaffolding was slower and is not the selected default.

The first jthread build failed on Apple libc++; explicit thread ownership and
joining resolved it. Twelve-worker small fixtures pass ASan/UBSan. Worker counts
1/2/4/6/12 match full seed 1 repeatedly; twelve workers match full seed 7.
Official source/dependency axes and final container evidence are recorded
separately. Official T/M/K/points and board IDs remain unknown.

Final offline linux/amd64 build and five full seed 1 sandbox runs pass using the
unchanged official process timer under ARM emulation. Library inspection finds
only libc/libm/loader, with libstdc++ and libgcc linked into the executable.
The final image is about 30 MB. `static-final.json` records S=187 via the official
formatter/count tooling; D=1, basis `declared, unverified`. This is a local static
measurement, not an official board result. Final default-worker sample seeds
2/3/42 also match the unchanged reference.

## Final native verification snapshot

Final source `283d027c9aaa46b6e07fcd7b38c3be70f4ceb4294165f28d4a2e9033f24df4ec`. Public full seed 1 repeated five times with
exact hashes: local middle-three mean 0.250590 s; peak wait4 RSS
311,558,144 bytes. Raw times: 0.255074, 0.256713, 0.252603, 0.243396, 0.244094.
Current-source tiny/sample seed 1, samples 2/3/42, independent full seed 7,
worked/targeted fixtures and small sanitizer checks all pass. Final native
verification is serial; these remain ARM64 process measurements, not board T/M.

## Clean archived release checks

From committed archive 47bb5276e4296119b3774b564a72f13a3692ffd0, no-cache
`linux/amd64 --network=none` build: 15.136 s, image 30,379,318 B.
Five full seed 1 runs and one full seed 7 run pass in the read-only/unprivileged
sandbox, with no unexpected root filesystem changes and all hashes exact.
Final runtime library inspection succeeds. These are ARM-host emulated checks.
Official tooling locally reports S=187, D=1 (declared, unverified).
This release checkpoint preceded the first live-board run recorded below.
Final holdout and judge results remain pending.

## First official live-board result — 2026-11-01

Commit `b4f72402d632209fd3f063d6e52f64d04fb97da1`, run `run-3051ba50d6d4`.
All check/build/correctness/timeout/sandbox gates passed. Public full seed 1,
native scoring box: T=0.275095 s, cgroup M=69,869,568 B,
S=187, D=1, K=11.411144, machine points=52.866460.
This is the live board's single public-data run, not the final five-run holdout
or judging. Raw receipt: `../benchmarks/board/2026-11-01-first-public-run.json`.

## A2 retained locally — parallel hour completion

Diagnostic at the original live-board source measured ingestion 0.213473 s and
serial sorting/formatting 0.141328 s (instrumented ARM64 development copy).
Hour tasks now sort disjoint group vectors and write disjoint ordered strings.
Five alternating full-seed-1 runs against b4f7240 gave local middle-three
means 0.301456 s baseline and 0.166677 s candidate; peak wait4 process RSS
311,328,768 and 301,236,224 B. First samples were cold/high (0.922/0.521 s);
all ten output hashes matched. Raw evidence: comparisons.jsonl, A2 hypothesis.
Worked/edge/empty/targeted sanitizer cases, sample seed 42, full seed 7 and
full public worker counts 1/2/4/6/12 all pass. This ARM result is provisional
until an actual new board run. No candidate K or points are inferred.

## A3 provisionally retained — contiguous latency storage

Count/prefix/scatter replaces small per-worker group vectors with contiguous
row and latency arrays. Worker scatter slices are disjoint; global cursors
become exclusive ends before independent hour sorts. All five alternating
public full outputs match; A2 middle-three 0.168248 s vs A3 0.128075 s.
Peak local wait4 RSS 301,891,584 vs 296,796,160 B. Raw comparisons.jsonl.
Worked/edge/empty/targeted sanitizer cases and sample 42 at twelve workers
pass. Independent full seed 7 at six workers also passes. Broader worker sweep
and amd64 proof pending.

## Intermediate Linux/runtime proof — 2026-11-01

Current source `2af732d7e2b6c2bcdeea34521cf3f391033859e1e81c43e1026b48ed58994f99`. Clean offline/no-cache amd64 build
passed in 7.495 s; image 30,383,456 B. One full public
run passed under uid65534, read-only root/data, no network, 8GB/no swap,
PID/scratch restrictions, exact output/hash and no unexpected filesystem diff.
Runtime libraries resolve. This is ARM-host amd64 emulation; no official
T/M/K is inferred. Raw `containers.jsonl`, artifact `.work/containers/a-34c90de37e`.
Unchanged official static tools now measure S=238, D=1
(declared/unverified dependency basis). Final candidate repeated sandbox and
fresh-data release checks remain separate from this intermediate checkpoint.

## Official live-board checkpoint — cec7edf

Run `run-ed6691497140`, ended 2026-11-01T18:20:56Z. All check/build/correctness/
timeout/sandbox gates passed. Public full native scoring-box T=0.131601 s,
cgroup M=53,760,000 B, S=238, D=1, K=16.768585,
machine points=61.224820. Registered in this feed: True.
Raw receipt `../benchmarks/board/2026-11-01T182140.380992Z.json`.
This is one real live public run, not final holdout/judging.

## A4 rejected — bulk delimiter calls

A detailed A3 native diagnostic found parsing dominates (parse0.177511 s,
merge0.002445, prefix0.000575, scatter0.006165, sort/format0.025470).
Replacing method/endpoint byte scans with bounded memchr calls did not help:
five alternating public middle-three means 0.104716 -> 0.109775 s; all hashes
exact. It stays isolated at `.work/candidates/a4.cpp`. No improvement claimed.

## A5 rejected — compact exact endpoint map

Five alternating native full outputs matched, but the middle-three time was
0.101308 -> 0.100895 s (less than half a percent), with inconsistent pairwise
order and cold first samples. Extra probing-table code is not justified by
this inconclusive difference. Preserve `.work/candidates/a5.cpp` and the raw
comparison; submitted A3 remains unchanged. Further small table rewrites stop
until a more substantial parser mechanism is supported.

## Official round-three result — 7f739ca

Run `run-ea6845912009`, 2026-11-01T18:37:16Z; all gates pass. T=0.098343 s,
cgroup M=53,035,008 B, S=238, D=1, K=19.450627,
points=64.446680. Registered=True. Raw receipt
`../benchmarks/board/2026-11-01T183856.248929Z.json`. Single live public run.

## Round-four exact-source proof

Digest `8ca9f5993b20617eab0b3e48d7ddf5512e5d0ce4a51656dc7c82fc2e9e345ffe`. Five public native runs pass; local middle-three mean
0.052247 s, max wait4 process RSS 304,594,944 B. These
ARM process samples include input mapping and are not scoring-machine axes.
Clean offline amd64 build plus FIVE restricted public runs and independent
full7 all pass, including exact outputs, libraries and filesystem diff checks.
Public container record `.work/containers/a-c0204208c6`; fresh `.work/containers/a-b1ec4c4a24`.
Unchanged official static tools measure S=238, D=1
(declared/unverified). Current native matrix gaps are being filled before
pushing. Actual round-four official run IDs/axes remain pending.

## A3 worker trial — twelve

On the current contiguous layout, paired native means six0.060984 ->twelve
0.053475 s, peak process RSS299,040,768 ->305,446,912 B. All hashes pass.
The current default is twelve for a real scoring-box experiment; this Mac
result does not establish the Intel optimum. Exact-source final matrix and
repeated/fresh restricted amd64 checks pass. Restore six if the actual
time/memory score does not justify twelve.

## Official round-four result — 1429e2e

Run `run-18e711ea44af`, ended 2026-11-01T19:24:42Z. All five gates pass.
T=0.089412 s, cgroup M=63,840,256 B, S=238, D=1,
K=19.656266, machine points=64.675051. Registered=True.
Raw receipt `../benchmarks/board/2026-11-01T192637.287985Z.json`.
Single public live run; final holdout/judging remain separate.

### PGO build-only trial — correctness complete, official effect pending

The unchanged A12 source is now built with a GCC-generated profile from an
offline sample seed8675309. The official generator is copied byte-identically;
its SHA and the training input SHA are recorded above. No training files,
Python or profiles enter the final runtime stage. Strict coverage/missing-profile
errors ensure the second compilation consumes compatible profiles.

The prototype image passed tiny1/sample1/2/3/42, all semantic fixtures, five
full1 sandbox runs and independent full7. Rebuilding the actual track directory
without cache/network took8.821986s, produced30,384,109 bytes, and passed five
full1 sandbox runs with empty filesystem diffs and exact published SHA. Full
non-MD context digests are in containers.jsonl, artifact a-de1b162318.
Formatted S248/D1 measured on byte-identical build inputs; a_ingest.py excluded
as a published copy by the unchanged official static tool. The current board
baseline remains1429e2e A64.675051; PGO benefit remains unknown until measured.

### Actual board receipt — f95f205

Run `run-dfae251f9662`: T0.086905s, cgroup M63,033,344B, S248, D1, K19.865532, machine points64.905010; all gates passed. Receipt benchmarks/board/2026-11-01T195245.141719Z.json. Only A build inputs changed since1429e2e. A score +.229959 in one run is within observed run variation; PGO benefit remains uncertain. B/C/D changes here are run variation because their runtime inputs are identical. D remains unregistered.

### Round-six candidate proof — 2026-11-01T19:56:23.493644+00:00

A13 selects exact nearest ranks in ascending order with nth_element, keeping each proven right partition. Duplicate percentile ranks reuse the selected value. Five paired ARM full1: .052602 ->.049621s; RSS307,134,464 ->304,037,888B. All native matrix checks, full7 five repeats, clean offline PGO build, five sandbox full1 plus full7 pass. Current S254/D1. Official effect pending; A12-PGO remains recoverable atf95f205.

Source/build-input hashes and exact proof references: benchmarks/round6-manifest.json. Raw paired samples and commands remain in comparisons.jsonl.

### Actual board trial5c216d1 — all gates passed

Run `run-8e4a8338a92d`: T0.079049s, cgroup M63,434,752B, S254, D1, K20.728396, machine points65.828285. Immutable receipt: benchmarks/board/2026-11-01T200313.061179Z.json. A13/B11/C9 registered overall57.177641 (geometric mean). D remains unregistered and its runtime bytes unchanged; its time change is variation, not a new algorithm. All A/B/C scored above the previous recorded best; retain these verified candidates.

## 2026-11-02 rewrite: per-hour exact aggregation

Local numbers are from a 4-vCPU Linux x86-64 container (GCC 13, `-O3 -mavx2`),
with peak memory taken from a child memory cgroup after a warm page cache.
They are for comparison between versions only; the board is the measurement.

| Version | Local T (4 workers) | Local peak M | Board T | Board M | Board points |
|---|---:|---:|---:|---:|---:|
| A13 (`5c216d1`) | 0.24 s | 58 MB | 0.0790 s | 63.4 MB | 65.83 |
| A14 two-pass scatter, AVX2 parser (`b6fb4a7`) | 0.12 s | 25 MB* | 0.0716 s | 42.1 MB | 67.26 |
| A14 trimmed, no O_DIRECT (`b1d2ca3`) | 0.13 s | 39 MB | 0.0603 s | 43.9 MB | 69.41 |
| A15 per-hour blocks (`71f59b0`) | 0.10 s | 19 MB | 0.0419 s | 33.8 MB | 74.61 |
| A15 in-place grouping (`35d1a42`) | 0.10 s | 19 MB | 0.0455 s | 29.5 MB | 74.21 |

\* O_DIRECT kept the output out of the local page cache. The board's `/out`
is tmpfs, so that saving does not exist there, and the code was removed.

What mattered, in order: storing no per-line rows (two-pass, then per-hour
processing); a branch-light AVX2 parser (about 2.6 mispredicts per line down to
0.4); a branch-free quickselect instead of `nth_element` (5.5M mispredicts at
full scale); a two-digit formatter; and writing finished hours without
blocking other workers. Identical code varied 0.042–0.050 s between board runs.

## Compact append allocation and equal-value selection trial

A7094ac6a... changes1024-entry chunks to256, sizes scratch by actual discovered
IDs, reserves per-hour result capacity and short-circuits an all-equal minimum
pivot partition. All percentiles remain exact. New packed-escape boundaries and
4097 equal latencies agree with frozen oracle. UBSan/bounds fixtures1/12, five
public-full pairs, full7 at1/12, offline image five full1/one full7 pass.
QEMU means.355806 ->.349413s inconclusive; process cgroup292,950,016 ->
294,174,720B includes pagecache/emulator and cannot estimate board M. S441/D1.
Native memory/time trial pending, prior source preserved036225c. Ledger
benchmarks/compact-a-six-b-trial.json. Next native ASan12 on actual committed source.

### Compact allocation native result — kept

b6ebbc8/run-3e475613b1fe: all gates pass, T0.027008s/M23781376B/S441/D1/K39.72327/79.95225points.
Memory fell from25,014,272B at036225c to23,781,376B. Runtime remains effectively
unchanged; small point gain is noisy but reduced allocation and the verified
all-equal guard justify retention. Native A12 ASan job41415961602 is pending.

## Four-track portfolio trial — native pending

Small <=24 groups now sort once for all three nearest ranks; new independent1..32-size fixture agrees with oracle.
Sourceb2470c2a995f821ac26a295fc992370278d04a1a8de430cedad3e8b713c53e61, S443/D1. Fixtures UBSan/bounds1/12,
fresh full7, five offline exact-image public runs and one fresh-image full7
pass. Five paired QEMU means0.404389 ->
0.393222s; not scoring-machine T/M. Exact
proof ledger benchmarks/four-track-portfolio-trial.json. Native ASan and board
effect pending; preserve prior source and select by complete native axes.
