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

## C6 unselected tradeoff — shared sharded membership

A shared exact name/country table under independent locks avoids private-name
replication and translation. Five paired public outputs pass, but local time
regresses: C5 0.331750 -> C6 0.505700 s. Local wait4 RSS falls
277,528,576 -> 219,758,592 B. This does not establish the official cgroup-memory
tradeoff. Keep the source isolated in `.work/candidates/c6.cpp`; additional
correctness and actual scoring-box axes would be needed before selection.

## Official live-board checkpoint — cec7edf

Run `run-94076763b165`, ended 2026-11-01T18:21:05Z. All check/build/correctness/
timeout/sandbox gates passed. Public full native scoring-box T=0.370713 s,
cgroup M=57,143,296 B, S=373, D=1, K=7.520145,
machine points=43.811309. Registered in this feed: True.
Raw receipt `../benchmarks/board/2026-11-01T182140.380992Z.json`.
This is one real live public run, not final holdout/judging.

## C6 clean official experiment candidate

The unused private-name bitmap fields/includes were removed from the isolated
C6 source. Its exact shared sharded membership algorithm is unchanged. Native
fresh full seed7 passes for the cleaned digest; the original C6 also passes
twelve-worker hostile grammar/sanitizer/sample42 checks. Clean offline amd64
build and three exact restricted public-full runs pass, with library and
filesystem checks. Runtime/process-memory comparison remains the earlier
local C5/C6 experiment; the official cgroup/time tradeoff has not been measured
yet. This is a deliberate board trial, not a claimed improvement.

Round-three source axes: S=348, D=1 (declared/unverified).
Clean build source digest matches `651a929898722a4320bad9ef995bf20580d855fe366ef9c2e7867d53ba4b6c97`; all three restricted
public full runs pass. Actual next board result remains pending.

## Official round-three result — 7f739ca

Run `run-da17fb250ea9`, 2026-11-01T18:37:24Z; all gates pass. T=0.253467 s,
cgroup M=15,585,280 B, S=348, D=1, K=11.916644,
points=53.807698. Registered=True. Raw receipt
`../benchmarks/board/2026-11-01T183856.248929Z.json`. Single live public run.

## C7 rejected — owner-partitioned name references

Each name hash selected one private owner; compact field references avoided
per-record locks, with a wide-offset fallback. Public hashes passed, but the
five paired ARM mean regressed0.080532 ->0.089120 s and process RSS increased
218,464,256 ->245,350,400 B. The extra reference storage and second name parse
do not pay in this test. Preserve isolated source/raw comparison; C6 remains.
These warm timings are only comparable within this block, not to earlier
blocks taken under different load or different source digests.

## Round-four exact-source proof

Digest `e22ce04db09dcf0ade071fd6dd6bac715f148986e477fbfd8769965377fe1134`. Five public native runs pass; local middle-three mean
0.088605 s, max wait4 process RSS 218,578,944 B. These
ARM process samples include input mapping and are not scoring-machine axes.
Clean offline amd64 build plus FIVE restricted public runs and independent
full7 all pass, including exact outputs, libraries and filesystem diff checks.
Public container record `.work/containers/c-4794a1bbf4`; fresh `.work/containers/c-c71fc76b28`.
Unchanged official static tools measure S=348, D=1
(declared/unverified). Current native matrix gaps are being filled before
pushing. Actual round-four official run IDs/axes remain pending.

## C6 worker trial — six

Sharing the name table removes C5's per-worker dictionary replication. Native
4-vs6 means0.081181 ->0.088334 s; 4-vs12 means0.081300 ->0.089488 s. All
outputs match and memory rises only slightly. Those Mac regressions do not
establish the Intel optimum: six is an explicit test of the box's six P-cores,
with exact-source native/fresh/repeated restricted-container proof. The
actual box must justify it before retention; four at7f739ca is preserved.

## Official round-four result — 1429e2e

Run `run-9c9de469bfe8`, ended 2026-11-01T19:24:52Z. All five gates pass.
T=0.206900 s, cgroup M=16,121,856 B, S=348, D=1,
K=13.100695, machine points=55.864717. Registered=True.
Raw receipt `../benchmarks/board/2026-11-01T192637.287985Z.json`.
Single public live run; final holdout/judging remain separate.

### Actual board receipt — f95f205

Run `run-bec25f0cbcb3`: T0.234699s, cgroup M15,962,112B, S348, D1, K12.324914, machine points54.539195; all gates passed. Receipt benchmarks/board/2026-11-01T195245.141719Z.json. Only A build inputs changed since1429e2e. A score +.229959 in one run is within observed run variation; PGO benefit remains uncertain. B/C/D changes here are run variation because their runtime inputs are identical. D remains unregistered.

### Round-six candidate proof — 2026-11-01T19:56:23.493644+00:00

C9 combines the independently tested C8 immutable atomic publication/country membership with private parser arenas. C8 five ARM pairs .088182 ->.081063s vsC6; C9 paired againstC8 .085414 ->.073596s. C9 RSS221,233,152B. All native fixtures/samples/full1/full7 matrix, new independently aggregated contention fixture, forced one-bucket overflow under sanitizers, x86 sanitizer fixtures, clean offline build, five sandbox full1 plus full7 pass. S390/D1. Table size is a growth heuristic with unbounded overflow, never a seed lookup/cap. Entries/key bytes/bits outlive all worker lookups. Official memory/time effect pending; C6 preserved at1429e2e.

Source/build-input hashes and exact proof references: benchmarks/round6-manifest.json. Raw paired samples and commands remain in comparisons.jsonl.

### Actual board trial5c216d1 — all gates passed

Run `run-751108049e14`: T0.170541s, cgroup M17,289,216B, S390, D1, K13.988312, machine points57.288266. Immutable receipt: benchmarks/board/2026-11-01T200313.061179Z.json. A13/B11/C9 registered overall57.177641 (geometric mean). D remains unregistered and its runtime bytes unchanged; its time change is variation, not a new algorithm. All A/B/C scored above the previous recorded best; retain these verified candidates.

C9 twelve-worker high-contention fixture also passed native ARM64
ThreadSanitizer on the exact current source, with independently derived expected
bytes. This is race/correctness evidence, never performance evidence. Raw
compiler/run commands: benchmarks/track-c/concurrency.jsonl.

## 2026-11-02 rewrite: quote-parity chunks, raw-byte name interning

Local numbers: 4-vCPU x86-64 container, GCC 13, `-O3 -mavx2`, peak memory from
a child memory cgroup.

| Version | Local T (4 workers) | Board T | Board M | Board points |
|---|---:|---:|---:|---:|
| C9 (`5c216d1`) | 0.40 s | 0.1705 s | 17.3 MB | 57.29 |
| Parity chunks + lock-free intern + atomic bitsets (`b1d2ca3`) | 0.23 s | 0.0612 s | 15.9 MB | 69.12 |
| Smaller tables, SWAR region ids (`a8f69d1`) | 0.23 s | 0.0614 s | 13.6 MB | 69.71 |
| Bit-word prefetch, inlined parsers (`c624b2e`) | 0.22 s | 0.0544 s | 13.8 MB | 70.85 |

Two bugs were found and fixed before any push: chunk cuts were initially
clamped to a stale `cut[0]`, which silently serialized the parse; and a
per-worker name arena was freed while other workers still compared against
it, which over-counted distinct names. The full-hash checks caught the
second. Prefetching both the name slot and the bitset word (eight records
ahead) cut single-thread parse time by about 18%.

## Bounded record lookahead native trial

Sourcea077037e5c98e31162f6a7ff06e58441c81c7b2efb63e1d113f335c9c45854f5; bounded16-record quote-parity lookahead gives independent parse
starts. Exact CSV semantics, interning and aggregates unchanged. New independently
aggregated vector/escape/BOM/CRLF/final-line fixture agrees with official oracle.
UBSan+bounds fixtures1/12, full1 paired hashes, full7 and exact offline image
five full1/one full7 pass. QEMU middle means0.413203 ->
0.455003s regress; this does not model native dependency latency.
One native board trial will decide; S418/D1. No improvement claimed.
Proofledger benchmarks/c-record-batch16-trial.json. Native ASan unavailable locally.

### Native record-lookahead trial rejected

ccc259f/run-b603168892dc: all gates pass, T0.049922s/M12931072B/S418/D1/K27.117219/71.662257points.
Prior identical-parent C at2d48af0 was.044476s/M13,066,240B/S379/73.190280.
Both extra scan overhead and source penalty lose. Restore463 C exactly; do not
pursue this lookahead line further. Receipt 2026-11-02T08:06:05.562657+00:00.

## Name-local country membership — native trial

Each immutable arena name owns its country words. Atomic fetch-or credits a
worker-local distinct counter exactly when the old bit was clear. The global
name-ID space and independent country-major bitsets are removed. Publication
is release/acquire; constructed atomic words are aligned and live until workers
join. Numeric fields and key bytes remain independent of atomic membership.
New country indices0/63/64/127/128/191/192/239/240 fixture agrees with frozen
oracle. UBSan/bounds fixtures1/12, forced one-hash collisions33workers, full7
1/12, offline image five full1/one full7 pass. QEMU five-pair means1.131957 ->
.910426s, local cgroup236,695,552 ->234,336,256B include emulator/page cache.
Header-comment update retained an identical verified release binary. S353/D1
versus379; native improvement unclaimed pending actual board. Full ledger:
benchmarks/prefix-and-inline-bits-trial.json. Native ASan pending manual CI.

### Native name-local membership result — kept

036225c/run-5390ab67f3f3: all five gates pass, T0.043319s/M12263424B/
S353/D1/K30.17635/73.983336points. This exceeds prior historical C
best73.246578 at6179d73/run-e8a44288f8f2. Against same parent baseline C at
2d48af0: .044476s/M13,066,240B/S379/73.190280. Memory/source gains are
measured; a single small time difference is noisy. Receipt2026-11-02T083443.958935Z.

## Four-track portfolio trial — native pending

Prefetch both immutable record header and exact-key line. Shared atomic slots are explicitly constructed before worker publication.
Sourcec18bd52a58ba17836285c5d7af96405067f6dcc0d6c1eec837de1a5fb4f869de, S357/D1. Fixtures UBSan/bounds1/12,
fresh full7, five offline exact-image public runs and one fresh-image full7
pass. Five paired QEMU means0.632634 ->
0.630418s; not scoring-machine T/M. Exact
proof ledger benchmarks/four-track-portfolio-trial.json. Native ASan and board
effect pending; preserve prior source and select by complete native axes.
