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

## Clean archived release checks

From committed archive 47bb5276e4296119b3774b564a72f13a3692ffd0, no-cache
`linux/amd64 --network=none` build: 4.903 s, image 30,384,308 B.
Five full seed 1 runs and one full seed 7 run pass in the read-only/unprivileged
sandbox, with no unexpected root filesystem changes and all hashes exact.
Final runtime library inspection succeeds. These are ARM-host emulated checks.
Official tooling locally reports S=350, D=1 (declared, unverified).
This release checkpoint preceded the first live-board run recorded below.
Final holdout and judge results remain pending.

## First official live-board result — 2026-11-01

Commit `b4f72402d632209fd3f063d6e52f64d04fb97da1`, run `run-be2e577bbf36`.
All check/build/correctness/timeout/sandbox gates passed. Public full seed 1,
native scoring box: T=2.585927 s, cgroup M=222,953,472 B,
S=350, D=1, K=6.028846, machine points=39.011708.
This is the live board's single public-data run, not the final five-run holdout
or judging. Raw receipt: `../benchmarks/board/2026-11-01-first-public-run.json`.

## B4 retained provisionally for the next exact-data experiment

Compact exact query-vocabulary lookup: five alternating native full-seed-1
middle-three means 2.429215 s (b4f7240) and 1.878239 s candidate. Peak local
wait4 RSS 515,375,104 and 519,028,736 B. All ten hashes match. Worked and
adversarial sanitizer fixtures, wide-sum case, sample seed 42 at twelve workers
and independently generated full seed 7 at six workers pass. Fingerprints
never substitute for complete string equality; the map is immutable before
index/query threads read it. Candidate static/amd64/board proof still pending.

## B5 provisionally retained — streaming exact phrase matching

KMP compares candidate token bytes directly against pattern terms, keeping
position/repetition semantics without constructing and hashing a token-ID
sequence. Five paired full public times are all faster; middle-three means
2.216936 -> 1.705743 s, peak local wait4 RSS 512,589,824 -> 510,623,744 B.
All outputs exact. Baseline ca2e75c. Independent full seed 7 and twelve-worker sanitizer fixtures pass. The added
overlap fixture initially had a wrong hand count: the untouched reference
and candidate agreed (document 0 also contains a b a). Its expected count
was corrected; corrected overlap and all 32 token-block offsets pass the
untouched reference and hand bytes with twelve-worker ASan/UBSan.
Official candidate publication still requires container checks.

## Intermediate Linux/runtime proof — 2026-11-01

Current source `078c200f83687009cf5f84e7d994fd09a3fdb0dd0f73a8aca2e779d764f9dc5e`. Clean offline/no-cache amd64 build
passed in 8.454 s; image 30,382,113 B. One full public
run passed under uid65534, read-only root/data, no network, 8GB/no swap,
PID/scratch restrictions, exact output/hash and no unexpected filesystem diff.
Runtime libraries resolve. This is ARM-host amd64 emulation; no official
T/M/K is inferred. Raw `containers.jsonl`, artifact `.work/containers/b-6cfd2971de`.
Unchanged official static tools now measure S=404, D=1
(declared/unverified dependency basis). Final candidate repeated sandbox and
fresh-data release checks remain separate from this intermediate checkpoint.

## Official live-board checkpoint — cec7edf

Run `run-30ea7d750cef`, ended 2026-11-01T18:21:01Z. All check/build/correctness/
timeout/sandbox gates passed. Public full native scoring-box T=1.136726 s,
cgroup M=223,354,880 B, S=404, D=1, K=8.896336,
machine points=47.460558. Registered in this feed: True.
Raw receipt `../benchmarks/board/2026-11-01T182140.380992Z.json`.
This is one real live public run, not final holdout/judging.

## B6 rejected pending diagnosis — isolated AVX2 tokenizer

A bounded 32-byte ASCII classifier compiled under the pinned linux/amd64
GCC and linked successfully, but the first worked fixture hung in the local
emulated runtime. It was stopped manually, with the failed cached-runtime
record retained. No score, speedup, or correctness pass is claimed. Submitted
B5 at cec7edf is unchanged and passed all official gates. Diagnose the isolated
source and prove token-boundary behavior before considering a new board push.

## B6-SSE official experiment candidate

Hypothesis: bounded 16-byte SSE2 ASCII folding/classification plus bulk LF
location reduces per-byte parser work on x86-64. ARM keeps its scalar path.
The AVX2 prototype hung under local Rosetta while indexing and was rejected.
The SSE2 source passes all worked/Boolean/phrase/UTF8/digit/wide-sum/overlap/
32-byte-block/dense-anchor fixtures on actual linux/amd64 code, plus small
Linux x86 ASan/UBSan at twelve workers, public full seed1 and independent
full seed7. Raw proof is in cached-runtime.jsonl with explicit fallback scope.
The first SSE conversion had intrinsic suffix compile errors; they were fixed
before the source that passed. No score or speed is inferred from emulation.
A clean offline build and three restricted public-full runs also pass, with
exact hashes and no unexpected filesystem diffs (`containers.jsonl`).
Publish as a real-box experiment against B5 at cec7edf; retain only if its
measured time/memory/source tradeoff justifies it. No candidate board ID yet.

Round-three source axes: S=448, D=1 (declared/unverified).
Clean build source digest matches `6ea3e83399bcfa329bcfb3fb1a23937a3477acdccdc8072c5e90eaf9e562e331`; all three restricted
public full runs pass. Actual next board result remains pending.

## Official round-three result — 7f739ca

Run `run-45214b11e35d`, 2026-11-01T18:37:21Z; all gates pass. T=1.046684 s,
cgroup M=225,148,928 B, S=448, D=1, K=9.113850,
points=47.985094. Registered=True. Raw receipt
`../benchmarks/board/2026-11-01T183856.248929Z.json`. Single live public run.

## B10 rejected locally — borrowed lowercase token views

Folding only tokens with capitals preserves public outputs, but five paired ARM
means regressed0.643171 ->0.712995 s, with no RSS benefit. Keep isolated source
and raw results; do not infer an x86 gain or select the candidate.

## B9 unselected — bounded short-key hash

Complete first/last bytes for terms <=16 bytes, standard hashing otherwise,
always full exact collision equality. Five public hashes pass. ARM paired means
0.635794 ->0.622926 s (about2%), with one regressing pair and broad cold-sample
spread. The extra code is not justified by this local result alone; no official
improvement is claimed and the submitted standard hash is preserved.

## B8 equivalent-output index capacity census

Instrumented candidate census matched the public hash. On ARM: vocabulary75,747,
private term metadata25,450,992 B, private sparse capacity57,378,860 B and dense
capacity23,468,016 B; merged sparse capacity40,319,084 B and dense23,567,024 B.
Retaining both private and merged storage is a concrete peak-memory cost.
These are allocation capacities, not measured cgroup peaks or scored timings.
Raw equivalent-output diagnostic is retained in profiles.jsonl.

## Round-four exact-source proof

Digest `cdba1869c9f1396ed1919b8252f2bf8d46357dccc397fe42a0125fd2685b407b`. Five public native runs pass; local middle-three mean
0.577124 s, max wait4 process RSS 437,436,416 B. These
ARM process samples include input mapping and are not scoring-machine axes.
Clean offline amd64 build plus FIVE restricted public runs and independent
full7 all pass, including exact outputs, libraries and filesystem diff checks.
Public container record `.work/containers/b-53ade5471a`; fresh `.work/containers/b-e4aa40ecba`.
Unchanged official static tools measure S=515, D=1
(declared/unverified). Current native matrix gaps are being filled before
pushing. Actual round-four official run IDs/axes remain pending.

## B8 retained for actual-box trial — direct hybrid storage and early release

The capacity census motivated eliminating redundant frequent-term postings
and releasing private term buffers immediately after their global merge.
Global actual frequencies select sparse/dense layouts and exact phrase anchors.
Five native public pairs: middle-three0.633509 ->0.590427 s, max process
RSS503,054,336 ->443,072,512 B. All hashes pass. Current source
`cdba1869c9f1396ed1919b8252f2bf8d46357dccc397fe42a0125fd2685b407b`
passes full current-source semantic/sanitizer/sample/independent-full matrix,
actual x86 sanitizer fixtures, clean offline build, five public sandbox runs
and fresh full7 sandbox. S515/D1 declared-unverified. Actual points pending.
