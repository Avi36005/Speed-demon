# Kryptonite — Speed Demon 2026

Independent C++20 entries for Track A (Ingest), Track C (Join), and Track B
(Index), implemented in that order. Branch: `speed-demon`. MIT licence.
Each track folder contains all its runtime/build inputs and has zero third-party
runtime dependencies. Default paths are `/data` and `/out` as the frozen specs
require. No generated datasets, answers or development binaries enter the images.

Read STATUS.md for the current handoff and SUBMISSION_REPORT.md for final evidence
and outstanding account actions. Each track's README explains its algorithm and
PERF.md links its measured progression. Raw records are under benchmarks/;
large artifacts and generated data stay in ignored `.work/`.

From this directory, using Python 3.11+ and a C++20 compiler:

```sh
python3.13 tools/dev.py a --data ../data/track-a/full-seed1 --label full-seed1 --repeat 5
python3.13 tools/dev.py c --data ../data/track-c/full-seed1 --label full-seed1 --repeat 5
python3.13 tools/dev.py b --data ../data/track-b/full-seed1 --label full-seed1 --repeat 5
python3.13 tools/suite.py a --fixtures --scale sample --seeds 2 3 42
python3.13 tools/suite.py c --fixtures --scale sample --seeds 2 3 42
python3.13 tools/suite.py b --fixtures --scale sample --seeds 2 3 42
```

These development tools locate the unchanged official references/generators in
`../official/speed-demon-2026`. The submitted track folders need no parent files.
The runner compares exact output bytes and retains first-difference reports on
failure. Timing runs exclude the oracle and hash comparisons. Local wait4 RSS
and native Mac timing are distinct from official cgroup M and scoring-box T.

Pinned base images must be prepared before building offline. Then:

```sh
docker build --platform linux/amd64 --network=none -t kryptonite-a track-a
python3.13 tools/container_check.py a
```

Use `c` or `b` for the other tracks. The container tool makes five exact-output
sandbox runs with the unchanged official process timer, checks root filesystem
diffs and library availability, and reports its ARM-host amd64 emulation clearly.
Official board performance and user registration/app/form state remain unknown
until confirmed. This repository has no organiser push destination.
