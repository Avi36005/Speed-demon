# Kryptonite — verified optimization checkpoint

The last board-tested runtime is **1429e2e**, with **56.089310 overall** across
registered A/B/C. All four track runs passed check/build/correctness/timeout/
sandbox. The requested92 machine-point target remains unmet; this is an
intermediate engineering checkpoint, not the requested final file handoff.

The current change is an isolated **A build-time PGO trial**. Its exact inputs,
training provenance and clean image proof are pinned in
[the trial manifest](benchmarks/pgo-a-trial-manifest.json). The PGO build has
not yet been scored by the board. B/C/D runtime sources remain1429e2e.

## Verified runtime

| Track | Implementation at1429e2e | Actual machine points | Real run |
|---|---|---:|---|
| A | Twelve whole-line parsers, contiguous latency groups, parallel exact sorting | 64.675051 | run-18e711ea44af |
| B | Six query/index workers, SSE2 tokenizer, compact query vocabulary, hybrid postings, exact KMP | 48.838903 | run-4a8ff5ad0553 |
| C | Six quote-aware CSV parsers, shared exact name/country membership | 55.864717 | run-9c9de469bfe8 |
| D | Four query workers, forward/reverse CSR, exact bidirectional Dijkstra | 44.723104 | run-48c61f525199 |

[Raw official receipt](benchmarks/board/2026-11-01T192637.287985Z.json) contains
all axes and gates. D is still registered:false and excluded from overall.
Scores are from single live public-data runs. Final hidden-holdout and judge
results remain unknown. Local ARM timings/RSS and emulated Docker timings are
kept separate from official scoring-box T and cgroup M.

## Verification evidence

[The four-track manifest](benchmarks/round4-manifest.json) pins the1429 runtime
and proof matrix: public tiny/sample/full1; independently generated sample2/3/42
and full7; hand-worked and hostile grammar cases; small ASan/UBSan checks;
worker sweeps; five deterministic native full runs; clean offline amd64 builds;
five restricted public-full sandbox runs plus independent full7 per track.
B's actual x86 SSE2 branch also passed sanitizer fixtures.

A's PGO prototype preserves that main source. A pinned build-only Python stage
runs the untouched official generator at sample seed8675309. Instrumented GCC
execution produces profiles, and a strict second compilation consumes them.
Only the binary enters the Debian runtime. Prototype tiny/sample1/2/3/42,
fixtures, five public-full and independent full7 checks passed. Rebuilding the
actual submitted track context offline/no-cache passed five public-full sandbox
runs; context digests, runtime libraries and empty filesystem diffs are retained.
S248/D1 was measured on byte-identical build inputs by the official static tool;
its published-copy rule excludes the byte-identical generator. Official effect
is pending. No input-derived training answers or data enter the final image.

C8/C9 and B11 remain isolated, unselected development experiments. Their tests
and comparisons do not certify the submitted source. STATUS.md records the
next concrete checks and raw evidence locations. Original oracle checkouts are
unchanged. The old report generator refuses to reuse stale release hashes.

## Reproduce

Use Python3.11+ and the documented workspace layout with the untouched official
checkout and public datasets adjacent to this repository. Every track is an
independent C++20 build context with zero third-party runtime dependencies.
Prepare all digest-pinned base images before the offline build.

```sh
python3.13 tools/dev.py a --data ../data/track-a/full-seed1 --label full-seed1 --repeat 5
python3.13 tools/suite.py b --fixtures --workers 12 --seeds 2 3 42
python3.13 tools/suite.py c --fixtures --workers 12 --seeds 2 3 42
python3.13 tools/suite.py d --fixtures --workers 12 --seeds 2 42
python3.13 tools/container_check.py a --runs 5
python3.13 tools/container_check.py b --runs 5
python3.13 tools/container_check.py c --runs 5
python3.13 tools/container_check.py d --runs 5
python3.13 tools/board_receipt.py
```

Container checks use network=none, read-only root/data, uid65534,8GB/no swap,
PID/scratch limits, exactly one output with its expected SHA, runtime-library
inspection and empty filesystem diffs. See per-track PERF.md for the commands,
source/input hashes and all retained/rejected comparisons.

MIT notices are retained. Repository access and A/B/C team registration are
evidenced by the board; individual-registration attestations remain outside
that feed. No D form update or visibility change has been performed. Freeze is
2026-11-02 15:00UTC/20:30IST; reserve the final two hours for verification.
The public-source requirement still needs attention before its stated deadline.
