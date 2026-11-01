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
