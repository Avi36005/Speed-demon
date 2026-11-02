# Kryptonite — current checkpoint

Updated 2026-11-01 20:06 UTC. Target: **92 overall live-board machine points**.
Registered latest/best overall: **57.177641** (calculated geometric mean). Target not achieved. The user requested
no new file/ZIP handoff until 92 is measured. Existing project ZIP is an outdated
historical release; do not present it as current.

## Repository and deadline

- Entrant repo: https://github.com/Avi36005/Speed-demon.git, branch `speed-demon`.
- Latest pushed/board-tested commit: `5c216d18b8063310c47295fce70d8408865b4b4a`.
- A/B/C registered. D is implemented, passed, and scored, but registered:false.
- Repository private at last check. MIT notice retained. No visibility change or
  new form submission performed. Do not attest to unverified registration facts.
- Freeze: 2026-11-02 15:00 UTC / 20:30 IST. Stop tuning by 13:00 UTC to reserve
  final proof. Public licence/visibility requirement still needs attention.
- User authorized real commits/pushes, then said goodnight/bye; no cancellation.

## Latest actual board run —5c216d1

Registered overall **57.177641**, all gates passed. Immutable receipt
`benchmarks/board/2026-11-01T200313.061179Z.json`; source proof round6-manifest.
A13/B11/C9 are retained because each exceeded its previous measured best.

| Track | Points | T seconds | cgroup M bytes | S | Run |
|---|---:|---:|---:|---:|---|
| A13 | 65.828285 | .079049 | 63,434,752 | 254 | run-8e4a8338a92d |
| B11 | 49.567895 | .976826 | 160,452,608 | 545 | run-3717e5314522 |
| C9 | 57.288266 | .170541 | 17,289,216 | 390 | run-751108049e14 |
| D3 | 46.037492 | 1.116187 | 210,255,872 | 239 | run-ed34233348c5 |

D remains unregistered; its unchanged runtime varied in time. Original baseline
43.243. Local ARM RSS/emulated times never stand in for official T/cgroup M.
Final five-run hidden-holdout and judge results remain unknown.

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

## Current B13 candidate proof passed, pending push

B13 uses exact16-bit gaps with uint32 escapes only for private sparse postings.
Final query representations/semantics stay unchanged. Five ARM pairs .561669
->.554578s are inconclusive in time, RSS423.61 ->392.23MB. New large-gap fixture
including exactly65535 and large first IDs passes native workers1/12 sanitizer
and actual x86 worker1 sanitizers. Independent full7 five repeats and full
current native matrix pass. Clean offline amd64 build, five full1 plus full7
restricted runs pass; S574/D1. round7-manifest pins all source/build/input proof.
Next: push reviewed B13-only runtime trial, capture actual score and retain or
revert relative toB11. A13/C9/D3 are unchanged from5c216d1.

C9 high-contention exact fixture passed ThreadSanitizer at12workers; raw proof
in benchmarks/track-c/concurrency.jsonl. New four-track README banner rendered
and visually inspected in an isolated headless browser; file remains nativeSVG.

Isolated/unselected `.work/candidates/c10.cpp` borrows unescaped name bytes from
the live mapping and copies only decoded scratch keys. Suite/paired comparison
active. B12 early tokenization exit was within noise and unselected. D4 radix
heaps regressed and were rejected. Keep all passing fallback commits.

No new ZIP/file handoff until actual92, as requested. No form or visibility
change has been performed. Continue prioritizing measured B index cost; stop
inconclusive paths and preserve the final verification window.

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
