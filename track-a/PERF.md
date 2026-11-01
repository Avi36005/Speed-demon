# Track A performance evidence

First baseline: mmap, input-backed endpoint keys, uint32 latency vectors, full
bucket sorting, one worker. Measurements will be recorded by `tools/dev.py` in
`benchmarks/track-a/experiments.jsonl`, including source digest and commands.

Official board runs: none. Official T, M, S, K and points: unknown.
Final container/runtime validation has passed; local RSS is not cgroup memory.

## 2026-11-01 native baseline

Source SHA-256: `21e0f155b9f5e076bfa80c957948d354d4539fb0ba5d4ed73495640c1e3c772a`.
Apple Clang 17, ARM64 macOS, `-std=c++20 -O3 -DNDEBUG -pthread`.
Public full seed 1 output matched the untouched reference and published hash
`495ca09682b9673ef836a506496177680bb4a3af54255174befddb1f83481cd3`.
Five local spawn-to-exit times: 1.158741, 0.744164, 0.721905, 0.703902,
0.734554 seconds. Mean of middle three: 0.733541 s. Maximum local process
RSS: 286,588,928 bytes, collected with wait4. This is not official T or M.
Docker was starting/preparing images during the session; these are provisional
development measurements, not a controlled comparison with later experiments.

Worked example, official edge fixture, empty input, percentile/case/prefix/long
endpoint fixture, tiny and sample seed 1 all matched. Small edge and targeted
fixtures passed ASan/UBSan. The first benchmark wrapper failed because sandboxed
`time -l` could not read `kern.clockrate`; output correctness had already passed.
That failed experiment is retained, and the runner now uses wait4 directly.

## A1 retained — independent whole-line workers

Each worker owns its endpoint/hour vectors. Cuts advance to a complete line
boundary; merging retains every latency and error before exact full sorting.

| Workers | Paired scalar mean (s) | Candidate mean (s) | Candidate peak RSS (bytes) |
|---|---:|---:|---:|
| 1 | 0.570089 | 0.631456 | 286,441,472 |
| 2 | 0.688375 | 0.493506 | 291,504,128 |
| 4 | 0.442366 | 0.252881 | 301,547,520 |
| 6 | 0.461982 | 0.234380 | 311,377,920 |
| 12 | 0.435864 | 0.229677 | 341,508,096 |

Five alternating runs per version, Apple Clang 17, ARM64, identical full seed 1.
All fifty outputs matched. Baselines drift across blocks; compare each candidate
to its own paired baseline. Six workers selected: twelve had no reliable time
advantage and used more RSS. No x86 worker-count advantage is inferred.
The one-worker parallel scaffolding was slower and is not the selected default.

The first jthread build failed on Apple libc++; explicit thread ownership and
joining resolved it. Twelve-worker small fixtures pass ASan/UBSan. Worker counts
1/2/4/6/12 match full seed 1 repeatedly; twelve workers match full seed 7.
Official source/dependency axes and final container evidence are recorded
separately. Official T/M/K/points and board IDs remain unknown.

Final offline linux/amd64 build and five full seed 1 sandbox runs pass using the
unchanged official process timer under ARM emulation. Library inspection finds
only libc/libm/loader, with libstdc++ and libgcc linked into the executable.
The final image is about 30 MB. `static-final.json` records S=187 via the official
formatter/count tooling; D=1, basis `declared, unverified`. This is a local static
measurement, not an official board result. Final default-worker sample seeds
2/3/42 also match the unchanged reference.

## Final native verification snapshot

Final source `283d027c9aaa46b6e07fcd7b38c3be70f4ceb4294165f28d4a2e9033f24df4ec`. Public full seed 1 repeated five times with
exact hashes: local middle-three mean 0.250590 s; peak wait4 RSS
311,558,144 bytes. Raw times: 0.255074, 0.256713, 0.252603, 0.243396, 0.244094.
Current-source tiny/sample seed 1, samples 2/3/42, independent full seed 7,
worked/targeted fixtures and small sanitizer checks all pass. Final native
verification is serial; these remain ARM64 process measurements, not board T/M.

## Clean archived release checks

From committed archive 47bb5276e4296119b3774b564a72f13a3692ffd0, no-cache
`linux/amd64 --network=none` build: 15.136 s, image 30,379,318 B.
Five full seed 1 runs and one full seed 7 run pass in the read-only/unprivileged
sandbox, with no unexpected root filesystem changes and all hashes exact.
Final runtime library inspection succeeds. These are ARM-host emulated checks.
Official tooling locally reports S=187, D=1 (declared, unverified).
Official board IDs, scoring-machine T/M/K and judge scores remain unknown.
