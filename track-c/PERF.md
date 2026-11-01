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
