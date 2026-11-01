# Kryptonite — current checkpoint

Updated 2026-11-01 19:56 UTC. Target: **92 overall live-board machine points**.
Registered best-per-track overall: **56.155708** (calculated geometric mean).
Latest f95f205 run overall:55.249768; the best scores include older B/C runs. Target not achieved. The user requested
no new file/ZIP handoff until 92 is measured. Existing project ZIP is an outdated
historical release; do not present it as current.

## Repository and deadline

- Entrant repo: https://github.com/Avi36005/Speed-demon.git, branch `speed-demon`.
- Latest pushed/board-tested commit: `f95f20526f97cb3f0d59ce38a730540aba614e23`.
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
New concurrency and bitmap-summary fixtures are included in the latest candidate
native matrix; the original1429 manifest remains immutable.

## Current candidate — round-six proof passed

Latest pushed is now f95f20526f97cb3f0d59ce38a730540aba614e23, A12 build-time
PGO only. Official A run-dfae251f9662 passed with .086905s/63,033,344B/S248/
64.905010 points. Small+.229959 vspriorA remains within known run variation.
All four gates passed. B/C/D runtime inputs were identical to1429, so their
new measurements are run variation. Immutable receipt195245.141719Z.

Working runtime changes, fully checked, pending next push:
- A13 exact ascending nearest-rank multi-selection with offline PGO. Native
  paired .052602 ->.049621s, S254/D1. Independent full7 five repeats pass.
- B11 dense bitmap exact count/sum/top-ten summaries. Time inconclusive
  .580397 ->.578411s, RSS441.04 ->420.43MB; explicit official tradeoff trial.
  S545/D1. New bitmap-boundary/partial-word/wide-sum fixtures pass native and
  x86 sanitizers at12workers.
- C9 immutable exact-name atomic publication with parser arenas. Two isolated
  mechanisms C8 thenC9 each improved ARM paired time; C9 .073596s vsC8 .085414.
  Full7 five repeats, contention fixture, forced one-bucket overflow sanitizer
  and x86 sanitizers pass. S390/D1. No fixed name-count limit.
- D3 unchanged. D4 monotone radix-frontier experiment passed semantics but
  regressed locally .493666 ->.553765s, higher RSS; rejected.

All candidate A/B/C runtime digests match clean offline amd64 builds, five
restricted full1 and one full7 sandbox runs plus a current final native matrix.
Exact context hashes/proof references: benchmarks/round6-manifest.json.
Next: diff review, commit/push the verified round-six trial; retain or revert
using actual board time/memory/source points. Target92 not achieved, no newZIP.

Isolated `.work/candidates/b12.cpp` tests early tokenization exit after a phrase
match; suite/sample42 passed; five pairs .555767 ->.552742s are inconclusive. It
remains unselected; stop this small query-microoptimization path for now.
Prior rejected/unselected candidates remain outside track folders. Preserve
1429 and f95f205 fallback commits. Do not benchmark two CPU jobs concurrently.

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
