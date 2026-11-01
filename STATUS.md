# Kryptonite — current checkpoint

Updated 2026-11-01 19:46 UTC. Target: **92 overall live-board machine points**.
Actual registered overall: **56.089310**. Target not achieved. The user requested
no new file/ZIP handoff until 92 is measured. Existing project ZIP is an outdated
historical release; do not present it as current.

## Repository and deadline

- Entrant repo: https://github.com/Avi36005/Speed-demon.git, branch `speed-demon`.
- Latest pushed/board-tested commit: `1429e2ebfbf01e46c24a09d209ee9a206df23e15`.
- A/B/C registered. D is implemented, passed, and scored, but registered:false.
- Repository private at last check. MIT notice retained. No visibility change or
  new form submission performed. Do not attest to unverified registration facts.
- Freeze: 2026-11-02 15:00 UTC / 20:30 IST. Stop tuning by 13:00 UTC to reserve
  final proof. Public licence/visibility requirement still needs attention.
- User authorized real commits/pushes, then said goodnight/bye; no cancellation.

## Latest actual board run (single public-data run)

Receipt: `benchmarks/board/2026-11-01T192637.287985Z.json`.
All check/build/correctness/timeout/sandbox gates passed.

| Track | Points | T seconds | cgroup M bytes | S | Run |
|---|---:|---:|---:|---:|---|
| A12 | 64.675051 | 0.089412 | 63,840,256 | 238 | run-18e711ea44af |
| B8-free | 48.838903 | 1.063800 | 159,985,664 | 515 | run-4a8ff5ad0553 |
| C6 | 55.864717 | 0.206900 | 16,121,856 | 348 | run-9c9de469bfe8 |
| D3 | 44.723104 | 1.259096 | 210,563,072 | 239 | run-48c61f525199 |

D=1 each (declared). D score excluded from registered overall. Original official
baseline overall43.243; previous measured checkpoints50.305507 and55.002670.
Unchanged A source varied substantially between runs; do not claim all gains
come from algorithms. Final holdout/judge scores remain unknown.

## Passing submitted implementations

- A12: mmap, twelve line parsers, exact endpoint keys, contiguous count/prefix/
  scatter latency groups, independent hour sorting/formatting, ordered write.
- B8-free: six workers, exact query vocabulary, SSE2 bounded ASCII tokenizer on
  x86 (scalar ARM), local sparse/dense postings, merge-and-free, strict LTR
  Boolean operations, rarest-anchor phrase checks with exact streaming KMP.
- C6: six quote-safe CSV parsers, shared sharded exact owned-name/country table,
  private integer aggregates, exact unescaping/BOM/cents semantics.
- D3: four query workers, two-pass forward/reverse CSR, exact bidirectional
  binary-heap Dijkstra, stale-frontier purges, touched-distance reset.

All exact runtime source hashes and verification matrix pinned by
`benchmarks/round4-manifest.json`. Current1429 runtime has passed native
worked/edge/sanitizer/tiny/sample1/2/3/42/full1/full7, deterministic repeats,
worker sweeps, clean offline amd64 image builds and five restricted full-public
container runs plus independent full7. B SSE2 also passed actual x86 sanitizer
fixtures. Mac process RSS/emulated Docker times are never official measurements.
New concurrent-name-membership fixture was added after that matrix; currentC6
still needs its explicit check before refreshing final-native manifest.

## Isolated experiments (not selected yet)

- `.work/pgo-a/`: exact A12 main, pinned Python build-only training stage with
  byte-identical official a_ingest.py, generated sample seed8675309. GCC atomic
  profile generation then strict profile use, only binary copied into Debian.
  Clean offline image build, five full1 and one full7 sandbox runs, every small
  fixture, tiny1/sample1/2/3/42 pass. S248/D1; generator excluded as published
  copy, root MIT retains required notice. Training input SHA and generator SHA
  recorded in track-a/PERF.md. Separate tag kryptonite/track-a:pgo-experiment.
  Actual track-a now contains the verified PGO build and generator. Its clean
  build and five full1 sandbox runs passed, with all context hashes retained in
  benchmarks/pgo-a-trial-manifest.json. Next: commit/push the controlled trial
  and wait for the real board result before retaining it. No x86 gain claimed.
- `.work/candidates/c8.cpp`: immutable exact-name entries release-published in
  monotonic overflow buckets with atomic country membership; no fixed name cap.
  All small grammar/ASan/UBSan/sample42 at12workers pass. New high-contention
  fixture and forced-one-bucket collision copy pass. Five alternating full1
  runs: C6 .088182 -> C8 .081063 s (ARM); RSS218.76 ->221.18MB. Source SHA
  8411ed03757a55c9a2381bd5df43ab9d5d99e60813e641f8dc033067db9ae201.
  Next: full7, x86 sanitizer and clean image checks before real-box trial.
- D3 diagnostic phase profile: graph construction .174475 s, queries .335458 s,
  write .000240 s (instrumented ARM; output matches full1). No new D change.
- `.work/candidates/c9.cpp` now keeps C8 lookup semantics but uses private
  parser arenas for immutable entries/keys/membership. All fixtures plus
  sample42 sanitizer checks pass; five pairs C8 .085414 -> C9 .073596s,
  RSS221.17 ->221.23MB (ARM). Full7/x86/clean image proof still pending.
- `.work/candidates/b11.cpp` summarizes dense Boolean results directly using
  exact popcount-weighted id sums and ten-smallest extraction; all small
  fixture checks have passed, sample42 and paired full1 comparison in progress.

Rejected/unselected experiments are preserved under .work/candidates and raw
comparisons: A4/A5, B9/B10, C7; B6 AVX2 stalled under emulation, root cause
unproven. Do not benchmark two CPU jobs concurrently. Preserve1429 fallback.

## Tooling and resumption

Use python3.13. Docker needs isolated anonymous config and explicit local socket:
`DOCKER_CONFIG="$PWD/.work/docker-public-config"`
`DOCKER_HOST=unix:///Users/avinashgehi3/.docker/run/docker.sock`.
Docker/gh/network calls require sandbox escalation with authorized justification.

`tools/dev.py --reuse-reference` reuses only fully hashed prior official oracle
outputs; candidate byte comparison still mandatory. `tools/container_check.py`
supports separate --context/--tag and records every non-MD build-context digest.
`tools/board_receipt.py` saves only this team with cache-busting/length checks;
extend build-input digest capture for PGO's generator. `tools/report.py` is an
OLD release generator and refuses changed runtime; do not overwrite evidence.
README/report need final current-source refresh after selection. No new archive.

Full prior status history: benchmarks/status-history/2026-11-01T194640Z.md.
