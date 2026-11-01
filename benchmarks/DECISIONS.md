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
