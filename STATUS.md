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

## GitHub round-three push — 2026-11-01T18:36:36.822697+00:00

`7f739cae9322023240a7225b73c077af5dc77fb5` successfully pushed. Only runtime
changes from cec7edf are B6-SSE and C6-clean. Both have clean repeated sandbox
proof, fresh-seed/fixture evidence and current static axes (B448 / C348, D1).
The next actual feed result is pending; do not infer improvement yet.

A4 rejected (0.104716 -> 0.109775 s local). A5 compact exact endpoint map
is the next isolated five-pair comparison, followed serially by D3 fresh
full-seed7 generation/reference comparison. D3 already passed five public
comparisons plus all-pairs/first-meeting/sanitizer/sample42 checks.
B8 combines SSE2 with B7 hybrid storage in an isolated source but has not
yet been tested. A3 and D0 in submitted source are preserved.

## Official round three measured — 2026-11-01T18:38:56Z

All 7f739ca gates passed. Registered best-three machine overall 55.002670:
A64.446680, B47.985094, C53.807698. C6 is retained: T0.253467 s and
M15,585,280 B versus C5 T0.370713/M57,143,296; score +9.996389. B6-SSE
score +0.524536 is modest and one public run only, so the mechanism's isolated
gain remains uncertain. A source/build files were IDENTICAL yet T moved from
0.131601 to 0.098343; never attribute that change to a new A algorithm.
D0 passed again, still unregistered; its score is not part of registered overall.

D3 independent full seed7 generation/reference check PASSED. Native all-pairs,
first-meeting, sanitizer/sample42 and five public comparisons already pass.
Next D3 requirements: worker matrix, clean amd64/runtime proof, source axes,
recorded official experiment. Only then consider a new D registration action.

B10 exact input-backed lowercase token views is the next isolated experiment.
B8 combined SSE/hybrid storage remains untested. A4/A5 rejected; no A change.
92 target not achieved; no new ZIP handoff.

## D3 selected for final candidate verification — 2026-11-01T18:56:08.859827+00:00

Current track-d/main.cpp now D3 with default FOUR (native time/memory compromise),
not yet committed/pushed. Fresh full7 at default2 already passed; current
default4 full7 plus five repeats is active via dev.py, then a B8 index capacity
census (equivalent output, no scored timing) runs serially. D3 worker5-pair
comparison: two0.774196 -> four0.483856 s, RSS200,949,760 ->223,444,992;
separate two0.767158 ->six0.425847 s, RSS200,916,992 ->281,526,272.
All hashes pass. Four selected to avoid the extra state memory for a modest
six-worker improvement. Official default selection remains provisional.

B10 input-backed lowercase-token prototype regressed on ARM (0.643171 ->
0.712995 s). It stays isolated; no adoption or x86 speed claim. The x86 SIMD
path would require its own checks, but this local result does not justify
prioritizing it. B8 capacity census will identify actual index storage costs
before any further allocation rewrite. B/C currently remain the real-board
passing 7f739ca sources; A remains unchanged A3.

PGO with official generated training is allowed by the master; no PGO source
or Dockerfile change has been made. Treat it only as a future hypothesis,
not active proof or an improvement. Final source/ZIP handoff still held at
user request until 92 is measured; actual registered best remains55.002670.

## Continued measured prioritization — 2026-11-01T19:03:54.498010+00:00

D3 default-four full7 plus five repeats passed, output hash66d63e1c...; the
full-public worker1/2/4/6/12 matrix passed. Clean container/static proof pending.
B8 capacity census proved private+global retained capacities, motivating
`.work/candidates/b8-free.cpp` to release each private term as soon as its
global term is merged. That source is still UNTESTED and UNSELECTED.
B9 short-key hash showed only about2% local gain, not justified for selection.
C7 name-owner queue regressed time/RSS and was rejected.

Important next trial: C6 is shared-state now, so C5's reason for keeping only
four parser workers no longer applies. Current exact-source C6 worker4-vs6 and
4-vs12 comparisons are active, then A3's current contiguous layout6-vs12.
Do not infer x86 optimal counts from Mac results; use a tested actual board
trial. A4/A5 remain rejected. No current A/B/C algorithm changes since7f739ca.

Isolated `.work/pgo-a/` has an UNTESTED PGO Dockerfile and an exact official
A generator copy: sample seed8675309 generated during build, profiles only,
final runtime copies only solution. It is not in the submitted track. Needs
allowed Python base preparation, offline build, hash checks, static exclusion
proof and actual-box comparison before any adoption.

## Scoring-box worker trials prepared — 2026-11-01T19:06:46.391180+00:00

Current uncommitted runtime changes from7f739ca: A3 default12 (was6), C6
default6 (was4), and D3 bidirectional default4 (wasD0). B remainsB6-SSE.
A3 Mac current-layout6-vs12 mean0.060984 ->0.053475 s, local RSS299,040,768
->305,446,912 B; all hashes exact. C6 Mac4-vs6 regresses0.081181 ->0.088334,
4-vs12 regresses0.081300 ->0.089488 with little RSS increase. These do not
establish the x86 optimum. C6 six-worker trial is specifically to use the
scoring CPU's six P-cores; retain only if actual board tradeoff improves.
A12 likewise needs actual measured memory/time axes before selection.

D3 worker/public/fresh/sanitizer proof is complete locally; pending clean
offline repeated-container checks and officialS. Next batch should verify
A12/C6default6/D3default4 with source-matched hashes, then commit/push only
the exact tested bytes. Preserve7f739ca if a trial regresses.

B8 incremental-release comparison runs isolated; if useful, run fixtures/
x86 sanitizers/full7/clean builds before considering it. B9 hash andB10 token
views are unselected. C7 owner queues rejected. PGO prototype still untested.

## B8 trial selected for regression verification — 2026-11-01T19:08:35.559824+00:00

Current working B main is B8-free (`cdba1869...`), not yet pushed. Five paired
ARM public means0.633509 ->0.590427 s, RSS503,054,336 ->443,072,512 B.
It directly promotes private frequent terms to bitmaps, uses exact actual-df
phrase anchors, and releases private term buffers after global merge. It
keeps the checked SSE2 tokenizer. All public comparison hashes pass.
Full semantic/sanitizer/worker12/sample42 and fresh full7 checks are active.

The runner now optionally reuses an untouched-reference output ONLY after
matching every input SHA256 and validating the saved passed reference command
and output hash. It still compares candidate and oracle BYTES and records
provenance. This avoids rerunning the same expensive reference for unchanged
full-seed inputs; new data or missing proof falls back to a fresh reference.
No runtime program reads these development outputs.

Current pending combined trial source: A12, B8-free, C6default6, D3default4.
All need exact-source clean offline/repeated public and representative fresh
container checks plus source-axis measurement before a new commit/push.
No actual gain is claimed for these unpushed bytes. Stable passing branch
remote remains7f739ca with registered overall55.002670.

## Round-four full proof — 2026-11-01T19:16:12.073885+00:00

All current A12/B8/C6default6/D3default4 sources passed: five native public
runs, independent full7 bytes, clean offline amd64 builds, FIVE restricted
public sandbox runs per track plus one fresh full7 sandbox run. B8 also
passes actual Linux x86 ASan/UBSan fixtures. Source axes A238/B515/C348/D239;
all declaredD1 unverified. Exact digests and evidence directories are pinned
in benchmarks/round4-manifest.json. No official round4 score yet.

The final-check helper now includes D and checks input hashes, hand-expected
bytes, actual worker count, compiler and architecture before reusing proof.
It previously could skip a requested worker check based only on source/label;
that verification weakness is corrected. `python3.13 tools/final_check.py` is
ACTIVE serially filling remaining tiny/sample/fixture/current-source gaps,
using only validated untouched-reference byte outputs when reusing an oracle.
Wait for its final passed record and source mapping before the round4 commit.

Next: commit/push the exact tested round4 source and docs, save official feed
results, and retain/revert A/C worker trials based on actual tradeoffs. Then
consider the isolated PGO build or further measured indexing work. Do not
claim the 92 target or produce a new source ZIP yet.

## Round4 ready to commit — 2026-11-01T19:19:50.253377+00:00

The final current-source matrix PASSED for all four digests, including
D3 tiny/sample1/sample2/3/42/full7, adversarial12-worker sanitizers and five
default-worker public repetitions. The helper checks source/input/worker/
compiler/architecture/hand bytes before reusing proof. All six sandbox runs
per track (five public +onefull7), clean offline builds and static axes pass.
Exact current sources equal round4-manifest and final-native.sources.
Proceed to commit and push A12/B8/C6-default6/D3-default4, then save actual
board results. Remote currently7f739ca with registered best55.002670.
A pinned allowlisted Python3.13 training base is now prepared; PGO prototype
remains unbuilt/untested and outside submitted tracks. No 92 claim or newZIP.
