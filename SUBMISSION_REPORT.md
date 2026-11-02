# Kryptonite — current verified checkpoint

Target: **90 live overall machine points**, aiming for top three. Target unmet.
Current official checkpoint463de7b (runtime42317bd) passed all gates on all four
registered tracks. Latest assembled-head overall72.804418; historical best-three
overall72.995658. These differ, and the freeze evaluates the actual final head.

## Official evidence

[Receipt2026-11-02T064445.120607Z](benchmarks/board/2026-11-02T064445.120607Z.json)
contains exact native axes, commits and gate results:

| Track | T s | M bytes | S | Machine points | Run |
|---|---:|---:|---:|---:|---|
| A | .027029 | 24,428,544 | 436 | 79.864346 | run-b6aeae4ae6b5 |
| B | .311542 | 46,931,968 | 775 | 66.167402 | run-6b5417c586c5 |
| C | .045172 | 13,053,952 | 379 | 73.025777 | run-723ade7b4bf4 |
| D | .485033 | 272,519,168 | 245 | 53.879383 | run-9076dddcddde |

D=1 each. All A/B/C/D are registered; D currently ranks fourth in this portfolio.
Final five-run holdout and judging are unknown. No current rank or90-point result
is inferred from earlier screenshots, emulator runs or historical bests.

## New B trial

Only B runtime changes relative to463de7b. For a no-OR expression, its positive
terms intersect and its NOT terms subtract. The program may therefore start
from a much rarer sparse positive AND operand and test exact memberships,
avoiding a large initial posting list. Expressions containing OR retain the
original strict LTR path. Source SHA and all build inputs/proof are pinned in
[the trial manifest](benchmarks/sparse-and-anchor-trial.json).

All fixtures pass UBSan halt-on-error and libstdc++ bounds assertions at12workers.
The new rare-AND/NOT/OR-revival fixture's independently derived sets agree with
the untouched reference. Five alternating full-public runs retain the published
hash. Independent full7 matches atworkers12/1. The exact amd64 image builds
offline/no-cache in9.978691s, occupies30,061,441B, and passes five full-public
sandbox runs plus full7, exact single output and empty filesystem diffs.
Formatted S793/D1 measured by the unchanged official tooling. Native effect
remains pending. QEMU screening2.551391 ->2.498198s is not a native gain claim.

Current A/C/D and the proposed B also passed current-source independent full7
checks. Raw source hashes, input hashes, untouched-oracle provenance, commands
and exits are in fresh-qemu.jsonl and per-track cached-runtime logs. Matching
native parent ASan/UBSan/small/generated checks from Claude remain preserved.
ASan itself fails during local QEMU allocator startup; new branch checks use
UBSan plus bounds assertions. This limitation is not relabelled as a passing
ASan run. Local emulated elapsed/RSS/cgroup values are never official T/M.

## Reproduction and remaining work

Each track is an independent C++20 Linux/x86 build context, zero third-party
runtime dependencies, pinned allowlisted GCC15/Debian13. Prepare base digests,
then build with network disabled. The entry images need no parent-workspace
files or development tools. On native x86 development hosts, use the existing
suite with `SD_CXX=g++ ASAN_OPTIONS=detect_leaks=0` and the untouched official
materials. This ARM Mac uses the source-recorded QEMU compatibility path.

```sh
docker build --platform linux/amd64 --network=none -t kryptonite-b track-b
python3.13 tools/container_check.py b --qemu --runs 5
python3.13 tools/qemu_fresh_check.py
python3.13 tools/board_receipt.py
```

The QEMU commands require prepared development-only emulator files described by
their pinned metadata in test records; they are not submission dependencies.
The old report generator refuses stale source hashes and must not overwrite
new evidence. Historic release manifests/reports remain unchanged.

Next: push this tested B trial, wait for its exact native board result, retain
or revert by complete axes, then continue measured portfolio work. Stop broad
rewrites by13:00UTC and reserve two hours for final assembled-head verification.
Freeze:2026-11-02 15:00UTC/20:30IST. MIT source must be public by21:30IST;
visibility still needs applicable authorization. Do not call an old ZIP current.
