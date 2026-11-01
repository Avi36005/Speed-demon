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
