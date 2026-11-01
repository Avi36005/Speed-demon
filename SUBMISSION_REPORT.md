# Kryptonite — optimization checkpoint

The initial A/B/C release was implemented, checked and then measured by the
actual live board at b4f7240. It scored overall 43.243 on the user's snapshot.
The requested target is 92 overall; that target is **not achieved**. This is
an intermediate checkpoint, not a final handoff.

The complete historical local release report is retained at
[initial-local-release-report.md](benchmarks/initial-local-release-report.md).
Its runtime hashes and claims apply to its recorded source, not automatically
to the current candidate. The legacy report generator now refuses to certify
changed runtime files with those old records.

## Current source and evidence

Use `git rev-parse HEAD` for the current commit; STATUS.md records active work.
Every track is self-contained, C++20, team Kryptonite, zero non-stdlib runtime
dependencies, MIT. Official checkouts and reference programs are unchanged.

| Track | Current algorithm | New native evidence | Remaining candidate proof |
|---|---|---|---|
| A | Parallel parse, contiguous count/prefix/scatter, parallel exact sorting/output | Five alternating full public matches; worker 1/2/4/6/12; sanitizer fixtures; sample 42; independent full seed 7 | Actual new board axes; final repeated candidate sandbox proof |
| B | Compact exact vocabulary, sparse/dense Boolean index, streaming KMP phrases | Five alternating public matches; small twelve-worker sanitizers including overlaps; full seed 7 | Actual new board axes; final repeated candidate sandbox proof |
| C | Four quote-aware chunk workers, compact owned name dictionaries and exact translated country bitsets | Five alternating public matches; general grammar/empty sanitizer cases; sample 42; full seed 7 | Actual new board axes; final repeated candidate sandbox proof |
| D | Scalar CSR and exact forward Dijkstra baseline | Hand cases, random all-pairs oracle, small sanitizers, samples 2/42, five full public matches | Fresh full seed, final repeated container proof, registration, actual board axes |

Local measurements are ARM64 Apple Clang 17 O3/NDEBUG/pthread. Comparison
records retain all raw samples, source digests, data hashes, commands and RSS
method under `benchmarks/track-X/`. PERF.md explains retained and rejected
experiments. ARM timings and process RSS are not scoring-machine T or cgroup M.

The first real A/B/C run IDs and all official axes are retained verbatim in
[the board receipt](benchmarks/board/2026-11-01-first-public-run.json). All
original live gates passed. Final five-run hidden-holdout and judge results
remain pending. Board access/registry for A/B/C is established; D selection is
not established by a push alone.

## Reproduction

Run from submission/ with Python 3.11+ and the untouched official checkout
next to it. `tools/dev.py` accepts a/b/c/d and compares exact reference bytes;
`tools/suite.py` generates independent data and small semantic fixtures.

```sh
python3.13 tools/suite.py d --fixtures --scale sample --seeds 2 42
python3.13 tools/dev.py d --data ../data/track-d/full-seed1 --label full-seed1 --repeat 5
python3.13 tools/container_check.py a --runs 1
python3.13 tools/container_check.py b --runs 1
python3.13 tools/container_check.py c --runs 1
python3.13 tools/container_check.py d --runs 1
python3.13 tools/board_receipt.py
```

Container checks build from a track folder alone with network disabled, pinned
allowlisted bases, amd64, read-only root/data, uid 65534, 8 GB/no swap, no
network, PID/scratch limits, exact file/hash, runtime-library inspection and
filesystem-diff checks. They are ARM-host emulation checks. Final candidates
need repeated runs and current static counts before a new final report.

Freeze: 2 November 2026 20:30 IST. Public recorded MIT source by 21:30 IST.
Keep a final verification window. Do not change repository visibility or
submit new attestations without the user's applicable authorization.

Current source clean offline amd64 checks have now passed once per track on the
full public input. Static S is A238 / B404 / C373 / D146, declared D=1 each.
Raw source-matching evidence: `containers.jsonl` and `static-round2.json` under
each track's benchmark directory. This remains an intermediate optimization
checkpoint; the user-requested 92-point handoff is not achieved.
