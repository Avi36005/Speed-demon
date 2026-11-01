# Track B performance evidence

Baseline: query vocabulary only, sorted document postings, scalar byte tokenizer,
left-to-right sorted set operations, rarest-term candidate phrase verification.
Full input reading, indexing and query evaluation share one process timer.

Machine-readable commands, source/input/output hashes and local samples are in
`benchmarks/track-b/experiments.jsonl`. Official board run IDs: none.
Official T/M/S/K/points: unknown; native Mac RSS and time are development evidence.

Baseline source SHA-256: `bc6f9e6feadc68cff045575f31cfbc4a5c952f231479d38f988ee377e2e98a9c`. Public full seed 1 matches
`f86973c210228141bcb6f1f1b10188ad2a9b65f71e7cd7cf4590b38de4e80927`. Five provisional native ARM64 samples (seconds):
15.747284, 13.841961, 15.367891, 15.966384, 20.162939. Peak wait4 RSS: 424,673,280 bytes.
Apple Clang 17, C++20, O3, NDEBUG, pthread. Image preparation and other local
verification overlapped the session, so these samples are not a controlled
comparison. Worked/targeted fixtures, wide ID sum, small sanitizer checks,
tiny seeds 1/2/3/42 and sample seed 1 passed.

## B1 — hybrid Boolean representation

Frequent terms and dense intermediate results use bitmaps; sparse sets retain
sorted postings. Every operator still runs left to right. Alternating full seed 1
means were 12.132796 s (baseline a45c753) and 10.003015 s (hybrid). RSS peaks
438,960,128 and 448,430,080 B. Raw samples have substantial spread, but the
candidate was faster in every paired iteration. This modest local gain does not
establish a scoring gain once source cost is included; no K is calculated.
All fixtures, small sanitizer checks, sample seeds 2/3/42, full seeds 1/7 pass.
Next: the unchanged index scan remains a major cost, so test independent
document-range index construction before making a final representation decision.

## B2 — independent document-range index construction

Complete document views are enumerated first. Each worker builds private sorted
postings; concatenating ordered ID ranges preserves sortedness without a sort.

| Workers | Paired B1 mean (s) | Candidate mean (s) | Peak candidate RSS (bytes) |
|---|---:|---:|---:|
| 1 | 9.314653 | 8.359076 | 438,534,144 |
| 2 | 14.846081 | 11.117755 | 483,524,608 |
| 4 | 12.038984 | 8.795779 | 494,567,424 |
| 6 | 9.117670 | 7.027117 | 504,152,064 |
| 12 | 9.535021 | 7.649231 | 536,395,776 |

Three alternating samples per version in each block, means of all three,
ARM64 Apple Clang 17. This is a worker exploration, not the official five-run
estimator. Baseline drift is substantial. Exact outputs pass all counts and
full seeds 1/7; small twelve-worker fixtures pass ASan/UBSan. The combined
B1/B2 version is preserved before a final diagnostic of remaining query cost.
