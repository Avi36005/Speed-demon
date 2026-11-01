# Track A performance evidence

First baseline: mmap, input-backed endpoint keys, uint32 latency vectors, full
bucket sorting, one worker. Measurements will be recorded by `tools/dev.py` in
`benchmarks/track-a/experiments.jsonl`, including source digest and commands.

Official board runs: none. Official T, M, S, K and points: unknown.
Container/runtime validation pending. Local RSS is not cgroup memory.

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
