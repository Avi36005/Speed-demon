# Experiment decisions

All measurements are local unless an actual board run ID is supplied. No board
runs exist yet. Full seed 1 inputs are verified against the official manifest.

## B1 — hybrid Boolean representation (in progress)

Baseline a45c753. Diagnostic phase sample: query parse 0.043 s, indexing 7.263 s,
evaluate/serialize 10.498 s on native ARM64 while other verification was active.
These are phase diagnostics, not controlled performance estimates.

Hypothesis: scanning long sorted postings repeatedly for common-term Boolean
operations dominates query work. Give frequent terms bitmaps and keep sparse
postings; switch the running result between representations while preserving
operator order. Sparse AND can probe the shorter list when sizes differ widely.
Tradeoff: extra bitmap memory and source size; keep sorted postings for phrases.
Decision pending differential tests and alternating baseline/candidate timings.

B1 result: 12.132796 → 10.003015 s local middle-three means; RSS 438,960,128 →
448,430,080 B. Spread is large; candidate wins each pair, but source cost makes
an official score gain uncertain. Preserve this revision (f63225a) and evaluate
parallel indexing, the remaining large phase, before finalizing the design.

## B2 — document-range parallel indexing (in progress)

Hypothesis: building query-term postings independently over disjoint ordered
document-ID ranges reduces the dominant index scan. Enumerate complete document
views first, use read-only vocabulary lookups, and concatenate worker postings
in range order. Tradeoff: extra posting containers, merge work and temporary
capacity. Test 1/2/4/6/12 workers, sanitizers and full seeds 1/7. Baseline f63225a.

B2 preserved at e9fc252 after all counts and full seeds 1/7 passed. Diagnostic
with six workers: query parse 0.0455 s, index 1.6726 s, query/serialization 6.2981 s.
Queries now dominate. Source/temporary memory costs and noisy worker means are
in PERF.md and comparisons.jsonl; no official scoring claim.

## B3 — independent query execution (final bounded tuning experiment)

Hypothesis: independent queries over immutable postings/bitmaps can use the
available workers. Claim small chunks dynamically to balance expensive phrases,
write each answer into its own slot, then serialize slots in original order.
Tradeoff: temporary result sets per worker and ~1.5 MB answer storage. Compare
against B2 with its index workers fixed at six; test counts 1/2/4/6/12 and full
seeds 1/7. Stop tuning after this experiment and complete the release matrix.

## C1 — global exact names and country bitsets (kept)

Baseline cc0f0e3. Diagnostic: lookup 0.0135 s, parse/aggregate 3.0318 s. Baseline
full local RSS is ~404 MB, which includes input mapping pages. Each country owns
string nodes for its distinct names. Hypothesis: intern each decoded name once,
then track its membership per country by a dynamic bitset. This should reduce
allocations and random string-node access while retaining exact collision
resolution and distinctions such as trailing spaces. Measure time and RSS;
the input mapping prevents local RSS from directly predicting cgroup memory.

Alternating full seed 1 comparison: baseline 3.056656 s / 405,258,240 B peak RSS;
candidate 1.126529 s / 219,250,688 B. Middle three of five local runs, identical
Apple Clang 17 flags, no overlapping task benchmarks. Every output matched.
Kept for the substantial measured time/allocation reduction. No K inference.

## A1 — independent line-boundary workers (in progress)

Baseline 886e42b. Diagnostic ingest 1.6098 s versus sort/serialize 0.2393 s under
concurrent verification. Hypothesis: independent endpoint/hour accumulation per
worker can reduce parsing/grouping time without shared counters. Split at whole
log-line boundaries, merge exact latency vectors before sorting. Tradeoff: more
temporary group containers, merge copies and larger source. Test 1/2/4/6/12
workers and compare against the preserved scalar baseline; keep only if useful.

A1 kept with six workers. Five-run middle-three local means by worker count:
1: 0.6315 s, 2: 0.4935 s, 4: 0.2529 s, 6: 0.2344 s, 12: 0.2297 s.
Baselines drifted between blocks; only paired baselines are comparable. Six-worker
paired baseline was 0.4620 s. Twelve workers were within noise of six and used
~341.5 MB RSS versus ~311.4 MB. Default six is provisional for the scoring CPU;
all measurements above are ARM64. Scalar construction remains available with
`--workers 1`. A rejected jthread build is retained; explicit thread ownership
works on this Mac's libc++ and GCC.

B3 kept: six-worker paired means 9.294215 s → 2.764396 s (see exact raw values
in comparisons.jsonl; prose rounded). Twelve-worker candidate 3.273262 s.
One worker regressed; six chosen provisionally. Stop tuning and finish release
verification. No x86 or official score improvement is inferred.

## Renewed tuning after official evidence — 2026-11-01

First official b4f7240 run: registered overall43.243; A52.866460/B39.011708/
C39.208881, with every gate passed. The next tested checkpointcec7edf reached
50.305507; 7f739ca reached55.002670. Every actual run ID and all axes are
archived under board/ and in each PERF.md. The requested92 target remains
unmet. Final holdout and judge scores remain separate.

Retained: A2 parallel hour completion, A3 contiguous count/prefix/scatter;
B4 compact exact vocabulary, B5 KMP phrases and provisionallyB6-SSE; C2 bulk
quoted runs, C4 owned exact name table, then C6 shared sharded membership.
C5 private dictionaries were correct but C6's actual memory/time tradeoff
scored better: C43.811309 ->53.807698, M57,143,296 ->15,585,280 B.
A3 had identical source/build bytes atcec7edf and7f739ca yet T0.131601 ->
0.098343; that change is run variation, not an invented A optimization.
B6-SSE's +0.524536 point single-run gain remains modest and not isolated
from ordinary variation.

Rejected/isolated: A4 memchr scans regressed; A5 compact endpoint lookup was
within noise; C3 borrowed intern keys were inconclusive; C7 owner queues
regressed time and memory; B9 short-key hash was only about2% locally; B10
borrowed token views regressed locally. AVX2B6 and its diagnostic hung under
local Rosetta, so they stayed out of submitted source. The SSE2 alternative
passed actual x86 sanitizer/boundary/full-data checks. No speculative gain
from an ARM or emulator time is counted as a box score.

Round4 candidate: A12 and C6-default6 are explicit scoring-machine worker
trials; Mac results do not decide the Intel optimum. B8 directly constructs
sparse/dense private indexes and frees each private term after merge; its
paired ARM mean0.633509 ->0.590427 s and RSS503,054,336 ->443,072,512 B
justify a real memory/time trial. D3 replaces scalar Dijkstra with a tested
bidirectional CSR design. It passes an independent all-pairs oracle and fresh
full seed7, plus full worker1/2/4/6/12 checks; four is the provisional default.
All current tracks pass the complete current-source native matrix, five
restricted public container runs and a separate full7 container check.
Actual round4 axes/points remain pending; revert any unjustified regressions.

Next bounded hypotheses: build-time GCC PGO on generator sample seed8675309
(prototype isolated, no scoring input in training), then profile B's remaining
index construction or D's construction/search split. Stop paths that remain
inconclusive and reserve the final two hours before freeze for verification.
