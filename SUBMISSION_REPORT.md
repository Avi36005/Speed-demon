# Kryptonite — verification and native-trial checkpoint

Target90 live overall machine points/top three remains unmet. All four tracks
are registered and pass native board gates. The latest ccc259f C lookahead trial
regressed and has been rejected; its results remain in immutable board evidence.

| Track | Machine points | Time | cgroup peak | Real run at `ccc259f` |
| :--- | ---: | ---: | ---: | :--- |
| A | 79.856533 | 0.026894 s | 24,780,800 B | `run-af057be8bbd9` |
| B | 65.461918 | 0.333285 s | 46,641,152 B | `run-aba2bc28e2a3` |
| C | 71.662257 | 0.049922 s | 12,931,072 B | `run-b603168892dc` |
| D | 52.201648 | 0.566995 s | 271,425,536 B | `run-aa2034569555` |

Latest trial overall72.088040; historical best-three73.012154. The freeze
scores the actual head. These are single public-data runs; final holdout and judging
are separate. Source-identical time changes are not algorithm improvements.

The new B empty-prefix simplification and C name-local atomic country bits are
pinned in [the trial manifest](benchmarks/prefix-and-inline-bits-trial.json).
Both pass independently derived/oracle-checked fixtures, UBSan halt-on-error and
bounds checks at1/12, full7 at1/12, public-full output hashes, exact offline amd64
images, five sandbox public runs and fresh full7. C forced collisions33workers and
all country-word boundaries also pass. Native performance is pending.

QEMU screening B1.914635 ->1.836980s and C1.131957 ->.910426s does not establish
Intel gains. Their local cgroup peaks include emulator costs. Local ASan cannot
initialize under QEMU. A bounded manual native Linux CI workflow is prepared for
changed B/C ASan/UBSan fixtures, fresh sample42 and full-public coarse diagnostics;
it is not recorded as passed until its exact commit/run artifacts are checked.
Unchanged A/D retain matching native parent sanitizer and fresh full7 proof.

Final committed-archive image checks, additional unused fresh seed, selected-head
watcher verification and release remain scheduled before15:00UTC/20:30IST. Stop
broad rewrites13:00UTC. MIT source must be public by16:00UTC/21:30IST; visibility
still requires applicable authorization. No old ZIP is presented as current.

Build each track from its own folder with pinned allowlisted bases and no network.
Native x86 checks use `SD_CXX=g++ ASAN_OPTIONS=detect_leaks=0 python3.13 tools/suite.py`.
Current source/build/input hashes, exact commands, outputs, failures and rejected
experiments remain in benchmarks and per-track PERF.md. STATUS.md lists next actions.
