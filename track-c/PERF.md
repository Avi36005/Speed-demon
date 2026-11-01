# Track C performance evidence

Scalar baseline: read-only mappings, quote-aware parser, direct region lookup,
per-country exact string sets, integer cents, no temporary note strings.
Evidence lives in `benchmarks/track-c/experiments.jsonl` outside this build context.

Official board run IDs: none. Official T/M/S/K/points unknown. Local timing and
process RSS will be labelled separately; they do not establish x86 performance.

Baseline source: `bfe6cfb1b4b096f0d6b07915190c6e913f985ef2d4f76e03506b630e79e7767e`.
Full seed 1 matches the reference and published output hash
`d8088b1cd42ba1338b1345b107b55db7b96ef1f5066f3ff64fd26a8912a0f536`.
Worked example, extra parsing rules (BOM as field data, unterminated final record,
leading BOM and final LF variants), header-only input and tiny/sample seed 1 pass.
Small rule fixtures pass ASan/UBSan. Five ARM64 native full samples:
4.511893, 4.764031, 4.183076, 3.323238, 2.572642 s; peak wait4 RSS 403,685,376 B.
The large spread overlaps image preparation; no controlled optimization claim
is based on these samples. Compiler: Apple Clang 17, C++20, O3, NDEBUG, pthread.

## C1 retained — global exact name IDs and country bitsets

Diagnostic lookup time was 0.0135 s versus 3.0318 s for parsing/aggregation.
Hypothesis: replacing per-country string nodes with globally interned names and
exact country membership bits reduces allocation and random node access.
Controlled alternating comparison against cc0f0e3 on the same full seed 1,
compiler and flags: local middle-three mean 3.056656 s → 1.126529 s;
max process RSS 405,258,240 → 219,250,688 bytes. All ten timed outputs match.
Raw samples, both source digests and exact commands: `comparisons.jsonl` in
`benchmarks/track-c/`. These are native ARM64 results; official memory and K are
unknown. The mapping contributes to RSS, so this is not an estimate of cgroup M.

The candidate passes all worked/rule/header-only fixtures, ASan/UBSan on small
fixtures, tiny seeds 2/3/42 and full seeds 1/7 against unchanged references.
Full seed 7 output: `da8b48aef3c54504a45cd4a8952916f4968ff5319a35fe2664e57b30655043db`.
The preserved scalar baseline also passed sample seeds 2/3/42 and full seed 7.

Final optimized offline linux/amd64 build and five full seed 1 sandbox runs pass
under ARM-host emulation. Runtime library inspection succeeds. See containers.jsonl
for build duration, exact image bytes, all timer records and output hashes.
Official tooling locally measures S=218 and declared D=1 (unverified basis),
recorded in static-final.json. This is not an official board performance run.

## Final native verification snapshot

Final source `1e8d10a65d2c5cdc1501ba3eb91da428cf653af822800d093e3765c1ac0f3bab`. Public full seed 1 repeated five times with
exact hashes: local middle-three mean 0.885086 s; peak wait4 RSS
220,971,008 bytes. Raw times: 0.875835, 0.919559, 0.933564, 0.859863, 0.845779.
Current-source tiny/sample seed 1, samples 2/3/42, independent full seed 7,
worked/targeted fixtures and small sanitizer checks all pass. Final native
verification is serial; these remain ARM64 process measurements, not board T/M.

## Clean archived release checks

From committed archive 47bb5276e4296119b3774b564a72f13a3692ffd0, no-cache
`linux/amd64 --network=none` build: 8.923 s, image 30,383,368 B.
Five full seed 1 runs and one full seed 7 run pass in the read-only/unprivileged
sandbox, with no unexpected root filesystem changes and all hashes exact.
Final runtime library inspection succeeds. These are ARM-host emulated checks.
Official tooling locally reports S=218, D=1 (declared, unverified).
This release checkpoint preceded the first live-board run recorded below.
Final holdout and judge results remain pending.

## First official live-board result — 2026-11-01

Commit `b4f72402d632209fd3f063d6e52f64d04fb97da1`, run `run-6b26d216e245`.
All check/build/correctness/timeout/sandbox gates passed. Public full seed 1,
native scoring box: T=1.133017 s, cgroup M=15,106,048 B,
S=218, D=1, K=6.083838, machine points=39.208881.
This is the live board's single public-data run, not the final five-run holdout
or judging. Raw receipt: `../benchmarks/board/2026-11-01-first-public-run.json`.

## C2 retained provisionally for the next exact-data experiment

Bounded memchr scans over quoted literal runs preserve commas, LFs and BOM
bytes and account for escaped pairs separately. Five alternating native
full-seed-1 middle-three means were 1.333985 s (b4f7240) and 0.988294 s.
Raw spread is substantial (baseline 0.803–1.560 s, candidate 0.786–1.188 s);
one paired run regressed. Treat the speed gain as provisional, not a scoring
claim. Peak local wait4 RSS 219,693,056 and 219,463,680 B. All hashes match;
worked/general-grammar/empty sanitizer fixtures, sample seed 42 and full seed 7
pass. Candidate static/amd64/board proof still pending.

## C3 rejected as a standalone change — borrowed intern keys

Borrowed plain decoded-name views eliminated copies but made the interned
comparison keys point into the 202 MB input. Five alternating local means
0.975728 -> 0.998996 s, with wide spread and a slower candidate aggregate.
All hashes pass. No reliable benefit is claimed. The source is preserved in
`.work/compare/c-986aaa64e3/candidate/`; the next test uses compact owned
unique keys while still borrowing transient plain-name lookups.

## C4 retained — compact owned-name interning

Exact unique decoded names are owned in an indexed vector; a compact probing
table filters by fingerprint and then compares complete bytes. Plain repeated
names are looked up through transient input views, avoiding per-record copies.
Five alternating full public means (middle three): C2 1.119033 s, C4 0.734436 s.
Peak local wait4 RSS 220,528,640 -> 225,165,312 B. All five pairs improve and
all hashes match. Worked/general-grammar/empty sanitizer fixtures, sample 42
and independent full seed 7 pass. Candidate official/static/container axes
remain pending. Raw comparison `.work/compare/c-ebcf8831b5/record.json`.

## C5 retained locally — proven CSV chunk boundaries and exact parallel merge

Quote parity over the actual bytes locates true record terminators, including
inside hostile grammar fixtures. Private exact dictionaries/aggregates parse
independently; local name IDs translate into a merged exact dictionary, and
country tasks OR translated name bits without races.

Five alternating C4/six-worker C5 public full runs: middle-three 0.717647 ->
0.262658 s, peak local wait4 RSS 222,707,712 -> 304,381,952 B. Exploratory
three-run blocks: two workers 0.683946 -> 0.448330 s, RSS 247,660,544 B; four
workers 0.721393 -> 0.329487 s, RSS 276,987,904 B. The three-run estimator
includes a cold first sample; do not compare it directly to middle-three-of-five.
Four selected provisionally to limit private-table memory; actual scoring-box
tradeoff is pending. Every hash passes. Twelve-worker grammar/empty/sample42
sanitizer checks and independent full seed 7 at four workers also pass.
Changing the default from six to four does not change the algorithm, but final
default-specific repeated/container/official measurements remain required.

## Intermediate Linux/runtime proof — 2026-11-01

Current source `4b3a997f06ba1ce96e6256375758370cd372cc067e684f0124578a0f587fe914`. Clean offline/no-cache amd64 build
passed in 9.455 s; image 30,392,968 B. One full public
run passed under uid65534, read-only root/data, no network, 8GB/no swap,
PID/scratch restrictions, exact output/hash and no unexpected filesystem diff.
Runtime libraries resolve. This is ARM-host amd64 emulation; no official
T/M/K is inferred. Raw `containers.jsonl`, artifact `.work/containers/c-6ea23756d2`.
Unchanged official static tools now measure S=373, D=1
(declared/unverified dependency basis). Final candidate repeated sandbox and
fresh-data release checks remain separate from this intermediate checkpoint.
