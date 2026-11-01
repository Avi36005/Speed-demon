# Kryptonite — Speed Demon entry status

Optimization resumed 2026-11-01. User target: **92 overall live-board machine
points**. Target not achieved; hold the new file handoff until it is achieved.
Freeze is 2026-11-02 15:00 UTC / 20:30 IST; reserve the final two hours for proof.

Branch `speed-demon`, remote `https://github.com/Avi36005/Speed-demon.git`.
Known passing official baseline: `b4f72402d632209fd3f063d6e52f64d04fb97da1`.
Current working changes: A2 hour-level parallel completion, B4 compact vocabulary
lookup and C2 bulk quoted-run scanning. All original baseline sources remain
recoverable. B4/C2 additional regression/container checks are still pending. Earlier passing
revisions and real commit timestamps are preserved. MIT; repository private
at the last check. Do not change visibility without the user's authorization.

## First official live-board baseline

Snapshot generated 2026-11-01T15:19:46Z, retrieved 15:19:52Z. Overall 43.243
from the user's board snapshot (rank 92 in that snapshot, not a permanent rank).
All three tracks passed check/build/correctness/timeout/sandbox. Registration for
A/B/C and successful board access to the private repo are now evidenced by the
public feed; final individual-registration attestations are not in that feed.

| Track | T seconds | cgroup M bytes | S | K | Machine points | Real run ID |
|---|---:|---:|---:|---:|---:|---|
| A | 0.275095 | 69,869,568 | 187 | 11.411144 | 52.866460 | `run-3051ba50d6d4` |
| B | 2.585927 | 222,953,472 | 350 | 6.028846 | 39.011708 | `run-be2e577bbf36` |
| C | 1.133017 | 15,106,048 | 218 | 6.083838 | 39.208881 | `run-6b26d216e245` |

Raw receipt: `benchmarks/board/2026-11-01-first-public-run.json`. All D=1.
Live board evidence is a single public-data run. Final five-run holdout and
judge scores remain pending. Local ARM process RSS and emulated Docker time
remain distinct from these official measurements.

## Passing implementations and previous verification

- A: six line-boundary workers, per-endpoint/hour uint32 vectors, exact sorting.
- C: scalar quote-aware CSV, exact decoded-name interning, country bitsets,
  dense region lookup, integer cents.
- B: six index/query workers, query-term sparse postings, dense Boolean bitmaps,
  strict left-to-right operations and exact phrase re-tokenization.
- Every runtime/build/metadata hash for the official baseline is archived with
  the board receipt and in `benchmarks/runtime-manifest.json`.
- Worked/adversarial fixtures, small ASan/UBSan, public tiny/sample/full seed 1,
  extra sample seeds 2/3/42, full seed 7, five native deterministic repetitions,
  A/B workers 1/2/4/6/12, clean offline amd64 builds, runtime libraries and five
  emulated restricted-container repetitions plus full seed 7 all passed before
  this tuning round. The board now supplies actual native scoring-box proof.
- Original official checkouts and references remain untouched.

## Active work / resume

Read the master and optimization brief in full again. The master now links
`../docs/codex-prompts/07-track-d.md`, which explicitly calls for reassessing
and implementing a measured fourth entry after preserving A/B/C. That brief
has been read; the D spec/reference/generator/tests still need full reads.
D is not implemented or registered; no D score is claimed.

1. Current-source native phase diagnostics for A, C, B completed serially via
   `python3.13 tools/profile.py a`, then c, then b. Do not run performance jobs
   concurrently. Save phase results in their existing profiles.jsonl records.
2. A2 measured and passed native regression/worker checks: 0.301456 -> 0.166677
   s, middle-three of five alternating runs. Candidate board/container proof
   remains pending. Next A experiment: compact count/scatter groups to reduce
   allocations and eliminate unused per-worker hour-vector storage.
3. Next weak-track opportunities: C decoded-name copies/hash lookups and scalar
   parsing; B repeated phrase re-tokenization and vocabulary lookup. Measure
   before keeping changes. Consider compact exact tables and safe parallelism.
4. Read D's complete authoritative materials and establish a correct baseline
   before advanced shortest-path work. Extend tools without breaking old proof.
5. Push only tested candidates (authorized earlier by the user), record actual
   new board IDs and all axes, retain known-passing source. No fake score or
   timing claims. The 92 target is aspirational until the official feed says so.

README, submission report and source ZIP currently describe the earlier local
release and need a final refresh after the new candidates are selected. The
old ZIP in the project folder remains the old passing runtime, not a new
optimized delivery. Do not call old docs current release proof.

## Resume checkpoint — 2026-11-01 17:54 UTC

User reiterated continuation and 92 target. No new file handoff. A3 contiguous
count/prefix/scatter candidate passed five alternating public-full comparisons:
A2 0.168248 s -> A3 0.128075 s, peak local RSS 301,891,584 -> 296,796,160 B.
A3 fixtures/worker-12/sample-42 checks are queued after D's first full baseline.
B4 and C2 passed adversarial fixtures/sanitizers plus full seed 7; commits
ca2e75c and 601e7a2 preserve them. D initial scalar CSR/Dijkstra source exists
but is uncommitted and not board-registered. D suites and five-repeat full
baseline are running serially. Candidate container checks and official pushes
are still pending. tools/report.py remains the OLD release generator and must
not overwrite new evidence or claim candidate release readiness.

## Verified intermediate source checkpoint — c391cbc

2026-11-01 18:08 UTC. Native current A3/B5/C4 checks passed, including full
seed 7. A3 workers 1/2/4/6/12 public full hashes pass. B's added overlap
fixture had a hand-count error, corrected after both candidate and untouched
reference agreed; corrected overlap and all 32 byte-block offsets pass small
twelve-worker sanitizers. C4 grammar cases/sanitizers/sample42/full7 pass.
D0 remains the correct scalar baseline. D1 indegree experiment was slower in
all three alternating pairs and was reverted; its raw source/results remain.

A clean source archive of c391cbc is in `.work/release-round2/`. Offline
amd64 checks are running serially via tools/container_check.py with one full
public run per track; final five-run candidate verification is still required
later. No candidate push yet, no new board score, and 92 is not achieved.

Isolated, unselected development candidates (not in submitted runtime):
- `.work/candidates/c5.cpp`: parallel CSV chunks with quote-parity boundary
  proof, private tables and exact translated bitset merges. Needs measured
  comparison against 0b98fcc plus worker/grammar/sanitizer/fresh-seed checks.
- `.work/candidates/b6.cpp`: bounded AVX2 ASCII token classification (guarded
  x86 target, scalar ARM fallback) and bulk LF scans. Needs amd64 build and
  boundary fixtures, sample/full hashes, then real-box comparison against B5.
- `.work/candidates/d2.cpp`: touched-distance reuse. Needs comparison against
  3772eb3 before selection. Do not run performance jobs concurrently.

Next: finish offline checks, measure current formatted S, push only the tested
c391cbc runtime checkpoint, record actual board output via board_receipt.py.
Then compare isolated C5, investigate B6 on amd64, and continue toward the target.
README/report now distinguish historical vs current proof. The old report
generator refuses changed sources; historical manifest remains immutable.

## Continuation — 2026-11-01 18:15 UTC

Docker preflight issue isolated: the normal config stalled before compiler-base
metadata. An isolated anonymous `.work/docker-public-config` (no credentials
copied or changed) plus explicit local socket successfully completed the exact
offline/no-cache A3 build. Obsolete stalled checks/fallback were stopped, and
the aborted preflight is honestly recorded in containers.jsonl. Clean checks
are now running serially with that config: A3, current C5, B5, D0 (one full
run each). No new push has been made yet; verify results before pushing.

C5 passed quote/BOM/general grammar/empty small sanitizers at twelve workers,
sample42 and full seed7 at four workers. Five paired public full at six workers
mean 0.717647 -> 0.262658 s, with higher RSS. Exploratory 2/4-worker blocks
are retained. Current `track-c/main.cpp` is C5 with provisional default FOUR,
not the scalar C4 from c391cbc. Default-four final repeated checks still needed.

`.work/candidates/b7.cpp` is a NEW UNTESTED isolated representation experiment:
private postings promote to per-range dense bitmaps, and global dense terms
no longer keep redundant full posting lists. Phrase anchors use actual df and
scan dense anchors exactly. Compare against c391cbc after container timing
finishes; do not select or publish it without full correctness evidence.
B6 isolated AVX2 candidate remains unselected. C5 source is the only current
runtime working-tree change since c391cbc. D0 remains unchanged.

Next commands after the container batch: inspect every latest containers.jsonl
record (source hashes must match); current S via unchanged static_axes.py;
commit/push tested A3/B5/C5 plus proof and fetch an actual new board receipt.
Then measure isolated B7, and use cached_runtime.py for AVX2 fixture checks if
needed. That helper is explicitly a cached-base runtime fallback, not clean
Dockerfile build evidence. Continue aiming for 92; target is not achieved.

## Ready for the next official experiment push — 2026-11-01 18:19 UTC

All CURRENT A3/B5/C5(default4)/D0 source digests match successful clean offline
amd64 build + one restricted full public run. Exact outputs, library inspection
and filesystem diffs pass. Official static tool S: A238, B404, C373, D146;
D=1 each, declared/unverified. The anonymous Docker config fixed the metadata
lookup path without changing account credentials or submitted Dockerfiles.

Next push contains these tested intermediate algorithms, not final 92-point
files. B7 and C6 are still isolated/unselected. C5's higher private-table memory
motivated C6's shared sharded exact name/country membership experiment. B7's
first local comparison improved time/RSS modestly; static tooling overlapped
early samples, so confirm a promising final version in an isolated repeat.
Do not claim either candidate is currently selected.

## GitHub checkpoint acknowledged — 2026-11-01T18:20:45.131072+00:00

`cec7edfdd6b56865e896a2a9953fa93fb4fdbe05` pushed successfully to origin
(speed-demon); remote advanced from b4f7240. User explicitly reiterated GitHub
commits, then said goodnight/bye; no new archive/file handoff was made because
the 92 target is not achieved. The first feed check was just before the watcher
noticed the new push; it still showed the old run. Check later without replacing
the queued commit until this official baseline finishes.

C6 exploratory exact shared-shard candidate: five pairs slower (0.331750 ->
0.505700 s) but local RSS lower (277,528,576 -> 219,758,592 B). Keep UNSELECTED
for a possible real-memory tradeoff experiment; local RSS cannot resolve its
scoring K. Exact public outputs pass; concurrency/edge/fresh-seed proof pending.

Next active command: cached_runtime.py b --source .work/candidates/b6.cpp
--fixtures plus full seed1 hash, six workers. This validates the actual AVX2
branch on linux/amd64; no speed gain is claimed from ARM native timing.
B7 remains isolated and unselected; five ARM pairs improved 1.663428 ->
1.516719 s and RSS 509,968,384 -> 488,226,816 B. Need meaningful regression,
fresh full seed and clean build before adoption.

## Official improvement confirmed — 2026-11-01T18:21:40Z receipt

Commit cec7edf passed all gates on all four track runs. Registered A/B/C
overall calculated from official machine points is 50.305507, up from
43.243. Actual A61.224820 / B47.460558 / C43.811309. D26.745647 also
measured and passed, but `registered:false`; do not infer a form update.
All real run IDs/axes are appended to PERF.md and archived in the dated feed
receipt. 92 not achieved. Do not replace these with local extrapolated scores.

C5 official T0.370713/M57,143,296/S373 confirms its memory cost limits its
gain. C6 local lower RSS may be worth an actual live tradeoff trial after
concurrency/fresh-seed checks. B5 official T1.136726/M223,354,880/S404 leaves
indexing and memory as major targets. B6 AVX2 verification still running;
inspect whether the cached compile, runtime or helper is stalled.

## Next scoring experiment preparation — 2026-11-01T18:31:38.763049+00:00

Official cec7edf overall 50.305507 is archived; user asked commits and said
goodnight. All score claims remain from the actual feed. No 92 handoff.

Current runtime working-tree changes (not yet committed/pushed):
- B6-SSE: `.work/candidates/b6-sse.cpp` copied to track-b/main.cpp. The
  128-bit SSE2 classifier passed ALL fixtures, max-32 token/block offsets,
  dense anchors, Linux x86 ASan/UBSan at twelve workers, public full seed1
  and independent full seed7. ARM retains its scalar fallback. AVX2 B6 and
  a bounded diagnostic hung inside indexing under Rosetta and were rejected;
  no root cause or actual-box speed claim is inferred. Initial SSE conversion
  had intrinsic-name compilation errors, corrected before the passing source.
- C6-clean: exact shared sharded membership locks with private numeric sums,
  copied to track-c/main.cpp. Original C6 twelve-worker grammar/sanitizer/
  sample42 and full seed7 pass; cleaned source full7 passes. Runtime slower
  locally but much lower RSS; this is an explicit official memory/time tradeoff
  trial, not a claimed improvement. Current clean repeated containers pending.

Active tool batch: clean offline B6-SSE then C6, three restricted public full
runs each. Only push if latest source digests and all exact gates pass. Then
measure new static axes; preserve cec7edf if a trial regresses on the real board.
B7 direct-hybrid layout remains ISOLATED, not selected. It passed native small
sanitisers, all fixture semantics, sample42 and independent full7. Do not
confuse B7 evidence with B6-SSE source hashes.

Isolated `.work/candidates/d3.cpp` now implements exact bidirectional Dijkstra
with valid-frontier stopping, two-pass forward/reverse CSR, touched resets and
two independent query workers. It is UNTESTED. Run all-pairs/first-meeting
fixtures before any full-data comparison. D0 in track-d remains unchanged.

## Round-three board trial ready — 2026-11-01T18:35:27.220058+00:00

Current B6-SSE and C6-clean exact source digests match clean offline builds
and three repeated restricted full public passes. B6-SSE also has actual
x86 Linux ASan/UBSan fixture and full7 proof. Source axes in static-round3.
Commit and push these concrete experiments next; candidate points are unknown.
D3 passed native all-pairs/first-meeting/sanitizer/sample42 checks and five
alternating public full comparisons: D0 4.557196 s -> D3 1.018434 s, RSS
207,028,224 -> 200,949,760 B. This changes search/layout/reuse/query workers
as one candidate design, so the individual contribution of each is unmeasured.
D3 remains isolated until fresh full data and clean-container proof.
A detailed diagnostic measured parse 0.177511 s, merge counts 0.002445,
prefix 0.000575, scatter 0.006165, sort/format 0.025470 (instrumented ARM).
A4 bulk delimiter scan is the next isolated comparison, not currently submitted.
