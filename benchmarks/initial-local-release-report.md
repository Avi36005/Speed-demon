# Kryptonite submission report

Evidence snapshot: 2026-11-01T14:30:04.397048+00:00.

Base commit at generation: `47bb5276e4296119b3774b564a72f13a3692ffd0` on `speed-demon`.
Clean archived build inputs: `47bb5276e4296119b3774b564a72f13a3692ffd0`. The runtime manifest below
pins the exact source/build/metadata bytes verified from that archive.
Documentation/evidence may be committed after this snapshot; `git rev-parse HEAD`
identifies the final delivery commit. The entrant destination is now confirmed:
`https://github.com/Avi36005/Speed-demon`, branch `speed-demon`. The user authorized
the push. The first push succeeded at remote commit `9ba13f4a2ca0b7693c9da1f5699c642126767fdd`
with 69 reachable commits, including 56 nonempty file imports. The original
verified development history is preserved; imports use their actual creation
times. The repo is private and `speed-demon` is its default branch. See
`benchmarks/publication.json` and `SUBMISSION_CHECKLIST.md` for evidence and the
remaining registration/app steps. Later receipt/documentation commits may follow.

## Implementation

C++20 entries A, C and B are implemented. MIT licence and starter notices retained.
All runtime code is in each independent track folder. Zero third-party runtime
dependencies; team metadata is Kryptonite / C++. Development artifacts are gitignored.

A uses whole-line private worker tables and exact latency sorting. C uses a scalar
quote-aware CSV parser, integer cents, global exact name IDs and country bitsets.
B uses query-only vocabulary, worker-local sorted postings, sparse/dense Boolean
operations in input order, and exact rare-anchor phrase verification.

## Correctness and packaging evidence

| Track | Public full seed 1 SHA-256 | Additional full seed 7 SHA-256 |
|---|---|---|
| A | `495ca09682b9673ef836a506496177680bb4a3af54255174befddb1f83481cd3` | `b384b9ddf5fa776ac8b9bbd10f935b5847f3b2cb52bcd2f7111fa8d00903d8c1` |
| C | `d8088b1cd42ba1338b1345b107b55db7b96ef1f5066f3ff64fd26a8912a0f536` | `da8b48aef3c54504a45cd4a8952916f4968ff5319a35fe2664e57b30655043db` |
| B | `f86973c210228141bcb6f1f1b10188ad2a9b65f71e7cd7cf4590b38de4e80927` | `a5a568281ee327b54011c43ac63e971b7a344948f58be60ee57ff07ef50f9c0a` |

Each current source passes worked/targeted fixtures, small ASan/UBSan checks,
public tiny/sample/full inputs, three additional sample seeds (2/3/42),
and an independently generated full seed 7. References and generators remain
unchanged. Full public inputs were rehashed against the published manifest.
A/B worker counts 1/2/4/6/12 produce identical full public outputs repeatedly.
C is deliberately scalar; quote-aware chunk splitting is therefore unnecessary.

Every final linux/amd64 image builds from its track alone with `--network=none`
and passes five public-full runs under the unprivileged/read-only sandbox.
The official process timer is compiled unchanged. Output names/hashes, exit 0,
root filesystem diffs, final runtime libraries, image size and entrypoint are checked.
Each final image also passes a full seed 7 sandbox run. The frozen official
spec/reference/generator repository remains at commit
`158894dcefde61f459c0adfae841900458dbeb7f`, with no working-tree changes.
These are ARM-host emulated Linux checks, not scoring-machine runs.

| Track | Final native mean (middle 3/5, s) | Local peak RSS (bytes) | Offline build (s) | Image bytes | Tool S | Tool D |
|---|---:|---:|---:|---:|---:|---|
| A | 0.250590 | 311,558,144 | 15.136 | 30,379,318 | 187 | 1 (declared, unverified) |
| C | 0.885086 | 220,971,008 | 8.923 | 30,383,368 | 218 | 1 (declared, unverified) |
| B | 2.230760 | 521,912,320 | 4.903 | 30,384,308 | 350 | 1 (declared, unverified) |

Native results use Apple Clang 17 / ARM64 / C++20 O3 NDEBUG pthread.
They are local spawn-to-exit measurements. RSS includes resident input mappings
and is not cgroup memory. Scored containers use the pinned GCC 15.3.0 toolchain,
with libstdc++/libgcc linked statically into Debian 13 runtime images.
S/D were measured locally using the unchanged official formatter/count tools.
D=1 is a declaration with an unverified basis, as reported by that tool.

## Reproduction

Run from the submission directory with Python 3.11+:

```sh
python3.13 tools/suite.py a --fixtures --scale sample --seeds 2 3 42
python3.13 tools/suite.py c --fixtures --scale sample --seeds 2 3 42
python3.13 tools/suite.py b --fixtures --scale sample --seeds 2 3 42
python3.13 tools/dev.py a --data ../data/track-a/full-seed1 --label full-seed1 --repeat 5
python3.13 tools/dev.py c --data ../data/track-c/full-seed1 --label full-seed1 --repeat 5
python3.13 tools/dev.py b --data ../data/track-b/full-seed1 --label full-seed1 --repeat 5
python3.13 tools/suite.py a --scale full --seeds 7
python3.13 tools/suite.py c --scale full --seeds 7
python3.13 tools/suite.py b --scale full --seeds 7
python3.13 tools/container_check.py a
python3.13 tools/container_check.py c
python3.13 tools/container_check.py b
```

Before offline builds, pull the exact GCC/Debian digests in each Dockerfile.
The official tools image preparation command is in `../official/speed-demon-2026/harness/README.md`.
Then measure each track with `static_axes.py --track-dir track-X --work .work/static`.
Raw commands, statuses, hashes, profiles, alternating comparisons and deterministic
run records are in `benchmarks/track-X/`. Large local artifacts are in `.work/`.

## Remaining external proof and account actions

- Official scoring-box T/M/K/judge scores and actual board run IDs: unknown; no submissions made.
- The unmodified full scoring harness requires Linux-host/Colima cgroup access.
  This Mac has no `/sys/fs/cgroup` host filesystem and no Colima; Docker Desktop
  uses cgroup v2 inside its VM. Official cgroup M and native i5-13500 measurements remain outstanding.
- Entrant repository is `https://github.com/Avi36005/Speed-demon`; source push is authorized.
  Registration, team form and Board App repository access remain unconfirmed.
  Changing visibility, installing the app and submitting forms have not been requested.
- The verified entry is now pushed to the entrant remote's `speed-demon` branch.
  Confirm the board observed the intended commit before freeze, 2026-11-02 20:30 IST.
  The recorded source must be public under MIT by 21:30 IST that day.

Implementation and available local checks are complete. Submission readiness is
**ready except for the named external account/board/scoring-machine actions and proof**.
No official performance or prize outcome is claimed.

Next useful experiments: profile B vocabulary lookup after parallel indexing,
and compare A exact multi-selection against sorting on the real scoring CPU.
Stop inconclusive paths; preserve the current verified revisions and final verification window.

## Exact runtime contents

See [runtime-manifest.json](benchmarks/runtime-manifest.json)
for SHA-256 of each main.cpp, Dockerfile and metadata file.
