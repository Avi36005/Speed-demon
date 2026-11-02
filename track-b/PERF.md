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

## Official round-four result — 1429e2e

Run `run-4a8ff5ad0553`, ended 2026-11-01T19:24:48Z. All five gates pass.
T=1.063800 s, cgroup M=159,985,664 B, S=515, D=1,
K=9.479339, machine points=48.838903. Registered=True.
Raw receipt `../benchmarks/board/2026-11-01T192637.287985Z.json`.
Single public live run; final holdout/judging remain separate.

### Actual board receipt — f95f205

Run `run-8d2bb9188ba0`: T1.183653s, cgroup M161,325,056B, S515, D1, K8.971637, machine points47.643585; all gates passed. Receipt benchmarks/board/2026-11-01T195245.141719Z.json. Only A build inputs changed since1429e2e. A score +.229959 in one run is within observed run variation; PGO benefit remains uncertain. B/C/D changes here are run variation because their runtime inputs are identical. D remains unregistered.

### Round-six candidate proof — 2026-11-01T19:56:23.493644+00:00

B11 summarizes final dense Boolean bitmaps without materializing all matching IDs. Six position masks plus exact popcounts compute the uint64 id sum; only the smallest ten are enumerated. Sparse/phrase summaries remain exact. Five ARM pairs .580397 ->.578411s are inconclusive in time; RSS441,040,896 ->420,429,824B motivates a scoring-box memory tradeoff trial. New independent bitmap-boundary/partial-word and wide-sum fixtures, native/x86 sanitizers at12workers, full7 five repeats, full native matrix, clean offline build, five sandbox full1 plus full7 pass. S545/D1. No official gain claimed yet.

Source/build-input hashes and exact proof references: benchmarks/round6-manifest.json. Raw paired samples and commands remain in comparisons.jsonl.

### B12 early phrase-tokenization exit — unselected

All small semantic/native sanitizer fixtures and sample42 passed. Five paired
ARM full1 runs againstB11: .555767 ->.552742s, RSS422,674,432 ->419,299,328B.
The time change is within observed run variation; do not select or claim an
x86 speedup. Keep B11 for the controlled scoring-box tradeoff trial.

### Actual board trial5c216d1 — all gates passed

Run `run-3717e5314522`: T0.976826s, cgroup M160,452,608B, S545, D1, K9.802975, machine points49.567895. Immutable receipt: benchmarks/board/2026-11-01T200313.061179Z.json. A13/B11/C9 registered overall57.177641 (geometric mean). D remains unregistered and its runtime bytes unchanged; its time change is variation, not a new algorithm. All A/B/C scored above the previous recorded best; retain these verified candidates.

### B13 exact private gap compression — official effect pending

Private sparse lists use16-bit monotone gaps plus a three-word escape for gaps
>=65535. Initial gaps are relative to each worker's first document; duplicate
terms per document remain suppressed before encoding. Promotion/merge decode
actual gaps exactly; final query indexes retain the proven uint32/bitmap layout.
Five ARM pairs againstB11: .561669 ->.554578s (timing inconclusive), peak process
RSS423,608,320 ->392,232,960B. Memory reduction motivates this real-box trial.
New independent large-gap/first-id/65535 boundary fixture passes workers1/12
ASan/UBSan; all actual x86 fixtures pass atworker1. Fresh full7 five repeats,
current native matrix, clean offline amd64 build, five public sandbox runs and
one full7 sandbox run pass. S574/D1 measured by unchanged official tooling.
All digests/raw evidence: benchmarks/round7-manifest.json; no official gain yet.

## 2026-11-02 rewrite: single scan, block runs, exact local phrase checks

Local numbers: 4-vCPU x86-64 container, GCC 13, `-O3 -mavx2 -mbmi2`, peak
memory from a child memory cgroup. Board numbers are single public runs.

| Version | Local T (4 workers) | Board T | Board M | Board points |
|---|---:|---:|---:|---:|
| B11 (`5c216d1`) | 2.0 s | 0.977 s | 160.5 MB | 49.57 |
| One scan, bitmaps + 16-bit runs (`b1d2ca3`) | 0.65 s | 0.281 s | 115.4 MB | 63.93 |
| Token-slot prefetch, branch-free merges (`a8f69d1`) | 0.62 s | 0.284 s | 112.8 MB | 63.76 |
| Flat query arrays (`c624b2e`) | 0.62 s | 0.248 s | 113.6 MB | 65.10 |
| Counting-sort blocks, df from runs (`3f87fd9`) | 0.58 s | 0.246 s | 97.0 MB | 65.95 |

Single-thread CPU split at `3f87fd9`: scan 1.30 s, queries 0.78 s, serial
parsing and indexing about 30 ms. Things that helped: replacing per-candidate
re-tokenization (7 µs per candidate, 214k candidates) with raw-text checks
around the rarest term; one tokenizing pass instead of two; and predicting the
dense terms from a sample, so their hits never enter the run buffers. Things
measured and not kept: a branch-free dense/sparse hit path (no change), 4096-
and 2048-document blocks (more runs, more memory), and a lower N/80 bitmap
threshold (5% faster but 12% more memory, so about neutral on K).

## Codex takeover — rare positive AND anchor trial

Baseline463de7b (runtime42317bd) run-6b5417c586c5: T.311542s,
M46,931,968B, S775, D1, points66.167402. All gates passed; raw receipt
2026-11-02T064445.120607Z. All four tracks are now registered.

First isolated attempt: index only BOOL terms after inline phrase matching.
All semantic/UBSan/bounds/full1/full7 checks passed. Paired QEMU means
1.972606 ->1.983256s were inconclusive; retain as unselected, no native gain
claimed. Source6433d5f92b19b1a07d0bd8619403bf050dda98a6d3730166c2be45c17f360ac6.

Selected for a native experiment: for an expression with NO OR, all positive
AND operands intersect and NOT operands subtract. If a sparse positive term
is at least16x rarer than the first term, enumerate it and test the remaining
memberships. Mixed OR expressions retain the original strict LTR path. This
avoids materializing large early sets; no query term or answer is compiled in.

All fixtures pass UBSan with halt-on-error and libstdc++ bounds assertions at
12workers, including a new independently derived rare-AND/NOT/OR-revival test
that agrees with the untouched oracle. Full1 five paired runs match published
bytes; full7 atworkers12/1 matches the untouched reference. QEMU paired means
2.551391 ->2.498198s are only screening evidence, not an Intel speed claim.
The exact image builds offline/no-cache in9.978691s, is30,061,441 bytes, and
passes five restricted full1 runs plus full7, uid65534, read-only root/data,
no network, empty filesystem diffs. Local QEMU image checks leave child T
unknown; recorded Docker elapsed includes startup. Measured S793/D1.

Local ASan itself cannot initialize its shadow allocator under this QEMU
backend; failed setup attempts are retained. Existing native parent ASan
records remain source-matched to unchanged code, and new branch checks use
UBSan plus bounds assertions. Complete current-source/build-input proof is
in benchmarks/sparse-and-anchor-trial.json. Actual candidate board effect is
pending; keep/revert only after native score evidence.

## Rare-AND trial rejected — native2d48af0

Receipt2026-11-02T075357.581616Z.json, run-650505a4fc9b: all gates pass,
T0.325392s/M46,702,592B/S793/D1/K20.550989/65.641637points. Parent463:
T0.311542s/M46,931,968B/S775/66.167402. QEMU screening did not predict a
native gain; source increased too. Reject and restore42317bd B exactly. This is
a measured regression, not an improvement. Source-identical other-track changes
are board variation.
