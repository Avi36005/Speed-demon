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

## B3 retained — independent query execution

Six-worker B2 phase diagnostic: index 1.67258 s, query/serialization 6.29807 s.
Hypothesis: distribute queries over immutable indexes, keep temporary results
private, and serialize disjoint answer slots after joining.

| Candidate workers | B2 with 6 index workers (s) | B3 mean (s) | B3 peak RSS (bytes) |
|---|---:|---:|---:|
| 1 | 8.545682 | 10.313355 | 451,903,488 |
| 2 | 9.737697 | 7.620825 | 487,505,920 |
| 4 | 11.635603 | 4.133578 | 497,434,624 |
| 6 | 9.294215 | 2.764396 | 504,283,136 |
| 12 | 10.882747 | 3.273262 | 531,251,200 |

Three alternating runs per version per block, all outputs exact, same native
ARM64 compiler/flags/data. Means use all three samples, not the official
five-run estimator. Baselines drift; use paired blocks. Six workers selected,
twelve did not help on this Mac. The one-worker variant regressed and is not
the selected default. Temporary sets and output slots add memory/source cost;
no K or official score claim is possible. This ends the tuning phase.

Final-source required matrix, native five-run samples and amd64 clean offline
checks follow in the structured evidence and SUBMISSION_REPORT.md.

## Final native verification snapshot

Final source `a27d48c237bbe733d395daead4a70b544498624c0394805d27f95fe0ce4bd8cf`. Public full seed 1 repeated five times with
exact hashes: local middle-three mean 2.230760 s; peak wait4 RSS
521,912,320 bytes. Raw times: 2.615541, 2.207020, 2.190346, 2.161126, 2.294912.
Current-source tiny/sample seed 1, samples 2/3/42, independent full seed 7,
worked/targeted fixtures and small sanitizer checks all pass. Final native
verification is serial; these remain ARM64 process measurements, not board T/M.
