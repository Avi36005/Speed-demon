<p align="center">
  <img src="docs/assets/kryptonite-banner.svg" alt="Kryptonite — Speed Demon 2026. C++20 entries for Ingest, Join, Index and Traverse." width="100%">
</p>

<p align="center">
  <strong>Exact results. Measured optimizations. Reproducible builds.</strong><br>
  Independent C++20 entries for the Speed Demon 2026 performance championship.
</p>

<p align="center">
  <a href="#the-entry">The entry</a> ·
  <a href="#verification">Verification</a> ·
  <a href="#local-measurements">Measurements</a> ·
  <a href="#build-and-run">Build &amp; run</a> ·
  <a href="#leaderboard-setup">Leaderboard setup</a>
</p>

---

> **Live-board checkpoint (`0023e65`, 2026-11-02):** all four tracks pass every
> gate. Registered A/B/C score **≈70.0 overall** (A 72.6, B 66.0, C 71.5; ≈70.7
> from each track's best run), up from 57.2. The requested 92 is not reached: it is above the board's best entry on
> every track. See [STATUS.md](STATUS.md) for every run and the scoring formula.

## The entry

**Kryptonite** implements four exact-output workloads with the C++20 standard
library and POSIX interfaces. Each track is an independent build context, with
all indexing, parsing and aggregation performed from the actual inputs at runtime.

| Track | Workload | Implementation | Required output |
| :--- | :--- | :--- | :--- |
| **A · Ingest** | Aggregate 3.1 million HTTP log lines | [Source](track-a/main.cpp) · [Approach](track-a/README.md) · [Performance log](track-a/PERF.md) | `summary.txt` |
| **C · Join** | Parse and join 3.32 million CSV records | [Source](track-c/main.cpp) · [Approach](track-c/README.md) · [Performance log](track-c/PERF.md) | `aggregates.txt` |
| **B · Index** | Answer 50,000 queries over 250,000 documents | [Source](track-b/main.cpp) · [Approach](track-b/README.md) · [Performance log](track-b/PERF.md) | `results.txt` |
| **D · Traverse** | Exact directed shortest paths for 2,000 queries | [Source](track-d/main.cpp) · [Approach](track-d/README.md) · [Performance log](track-d/PERF.md) | `distances.txt` |

| Language | Runtime dependencies | Submission branch | Licence |
| :--- | :--- | :--- | :--- |
| **C++20** | **0 third-party dependencies** | [`speed-demon`](https://github.com/Avi36005/Speed-demon/tree/speed-demon) | [MIT](LICENSE) |

### Engineering decisions

**A — one hour at a time.** Pass one records which hours start a line in each
16 KiB block. Pass two answers each hour from only its blocks, with AVX2
endpoint scanning, exact zero-padded key comparison and in-place grouping.
Exact nearest ranks come from a branch-free quickselect. No global latency
array is kept.

**C — exact parallel CSV.** Quote parity gives exact chunk boundaries despite
embedded LFs. Names are interned by their raw quoted bytes (unescaping is
injective) in a lock-free shared table with prefetched lookups. Distinct names
use atomic per-group bitsets; amounts are integer cents parsed with SWAR.

**B — one tokenizing pass.** AVX2 lowercasing and classification, prefetched
fingerprint lookups of the query vocabulary only, bitmaps for terms a sample
predicts to be common, and 16-bit block runs for the rest. Boolean operators
run strictly left to right; phrases are verified exactly in the raw text around
the rarest term.

**D — bidirectional Dijkstra.** Parallel CSR build, 32-bit distances with an
exact side map for very large values, and a stopping rule that treats a
meeting only as an upper bound.

## Official live-board measurements

| Track | Machine points | Time | cgroup peak | Commit |
| :--- | ---: | ---: | ---: | :--- |
| A | 72.575434 | 0.053169 s | 29,184,000 B | `0023e65` |
| B | 66.035647 | 0.245401 s | 95,227,904 B | `0023e65` |
| C | 71.454685 | 0.053050 s | 12,742,656 B | `0023e65` |
| D | 53.724257 | 0.496534 s | 272,211,968 B | `0023e65` (unregistered) |

Single public-data runs; identical code varies by up to ~15% in time between
runs. Final holdout scoring and judging are separate.
[Receipts](benchmarks/board) ·
[Kryptonite on the live board](https://speeddemon.dev/board/#kryptonite-96429f)

## Verification

The matrix below applies to the four-track `5c216d1` runtime, pinned in the
[round-six manifest](benchmarks/round6-manifest.json). Later candidates have
separate evidence in the performance logs and [current status](STATUS.md).

| Check | Evidence |
| :--- | :--- |
| **Byte-exact public results** | Tiny, sample and full seed 1 match the untouched references; full inputs and outputs match published hashes. |
| **Additional generated data** | Sample seeds **2, 3, 42** and independently generated full seed **7** match the official references. |
| **Failure-prone semantics** | Official worked examples, hand-computed fixtures, boundary cases, quoted CSV, Unicode separators and repeated phrases. |
| **Memory safety checks** | Small development builds pass **AddressSanitizer + UndefinedBehaviorSanitizer**. |
| **Deterministic output** | Five repeated native full-input runs per track; A/D also checked across **1, 2, 4, 6, 12** workers. |
| **Packaging and sandbox** | Clean, offline `linux/amd64` builds; five public-full container runs and one full seed 7 run per track under the required sandbox limits. |

Local Linux container checks used **amd64 emulation on an ARM64 Mac**. The
real live board supplies native scoring-machine measurements for its recorded
commits. Local process RSS and emulated timings remain separate from those axes.

[Full verification report](SUBMISSION_REPORT.md) ·
[Test coverage](tests/README.md) ·
[Runtime manifest](benchmarks/round6-manifest.json)

## Local measurements

The following are **historical initial-release development measurements on
ARM64 macOS**, using Apple
Clang 17 with C++20, `-O3`, `-DNDEBUG` and `-pthread`. Time is the mean of the
middle three of five process runs on the public full seed 1 dataset.

| Track | Local elapsed time | Peak process RSS | Formatted code lines |
| :--- | ---: | ---: | ---: |
| **A · Ingest** | **0.250590 s** | 297.1 MiB | 187 |
| **C · Join** | **0.885086 s** | 210.7 MiB | 218 |
| **B · Index** | **2.230760 s** | 497.7 MiB | 350 |

Process RSS includes resident input mappings and is distinct from official
cgroup memory. Code lines were measured locally with the organisers' static
tooling. Each track declares dependency weight **D = 1**, reported by that tooling
as **declared, unverified**. The official live-board measurements above apply
to their recorded commit. Current tuning comparisons are in each PERF.md.

The initial local amd64 runtime images were approximately **30 MB**; the live
board reported approximately **119 MB** images. Each uses an allowlisted,
digest-pinned GCC 15 build stage and Debian 13 runtime. The C++ and GCC support
libraries are linked statically; final runtime library checks pass.

Raw timings, source digests, commands, rejected experiments and measurement
methods are retained in [benchmarks](benchmarks) and the per-track performance logs.

## Build and run

### Build an independent track

Prepare every exact base-image digest listed in the track's Dockerfile before
the offline build. A's PGO trial adds a build-only Python training stage. Then build from that track folder alone:

```sh
docker build --platform linux/amd64 --network=none -t kryptonite-a track-a
docker build --platform linux/amd64 --network=none -t kryptonite-c track-c
docker build --platform linux/amd64 --network=none -t kryptonite-b track-b
docker build --platform linux/amd64 --network=none -t kryptonite-d track-d
```

### Run Track A

Place the official `requests.log` in `data/track-a/full-seed1/`. The public
datasets and checksums are linked from the [official board](https://speeddemon.dev/board/).

```sh
mkdir -p .out/a
chmod 777 .out/a

docker run --rm --platform linux/amd64 \
  --network=none --read-only \
  --memory=8g --memory-swap=8g --pids-limit=512 \
  --cap-drop=ALL --security-opt=no-new-privileges \
  --tmpfs /tmp:rw,size=1g,exec --user 65534:65534 \
  -v "$PWD/data/track-a/full-seed1:/data:ro" \
  -v "$PWD/.out/a:/out:rw" \
  kryptonite-a
```

The result is `.out/a/summary.txt`. Track C reads `records.csv` and `regions.csv`;
Track B reads `docs.txt` and `queries.txt`. Each writes the output named in the
track table. Input is read-only under `/data`; `/out` holds exactly one output file.

### Reproduce development checks

The development runners use the materials workspace layout: this repository at
`submission/`, official tools at `../official/speed-demon-2026/`, and public
datasets at `../data/track-*/`. Submitted track images need no parent files.

```sh
python3.13 tools/dev.py a --data ../data/track-a/full-seed1 --label full-seed1 --repeat 5
python3.13 tools/dev.py c --data ../data/track-c/full-seed1 --label full-seed1 --repeat 5
python3.13 tools/dev.py b --data ../data/track-b/full-seed1 --label full-seed1 --repeat 5

python3.13 tools/suite.py a --fixtures --scale sample --seeds 2 3 42
python3.13 tools/suite.py c --fixtures --scale sample --seeds 2 3 42
python3.13 tools/suite.py b --fixtures --scale sample --seeds 2 3 42
```

Python **3.11+** and a C++20 compiler are required for native development checks.
`tools/container_check.py` reproduces the local emulated container checks.
Correctness comparison and oracle execution are outside the candidate timing
interval; repeated timed runs include candidate startup, processing and output.

## Leaderboard setup

The source is already pushed to
**[Avi36005/Speed-demon](https://github.com/Avi36005/Speed-demon)**.
A/B/C registry and repository access are confirmed by the scored board runs.
The setup procedure, including changes to entered tracks, is documented in the
[official kickstart](https://speeddemon.dev/kickstart/):

1. Every team member completes [individual registration](https://tally.so/r/q4redG).
2. Grant the [Speed Demon Board App](https://github.com/apps/speed-demon-board)
   access to `Avi36005/Speed-demon`.
3. Submit the [team form](https://tally.so/r/Ar7g9z): **Kryptonite**, **C++**,
   tracks **A + B + C**, this repository URL, and the real contact/member details.
4. Check the [live board](https://speeddemon.dev/board/) after registry and queue
   processing. Save actual run IDs and results in the relevant performance logs.

**[Copy-ready submission details](SUBMISSION_DETAILS.txt)** ·
**[Complete submission checklist](SUBMISSION_CHECKLIST.md)**

The repo can stay private before freeze when the Board App has access. Confirm
the final branch commit was observed before **2 November 2026, 20:30 IST**.
Make the recorded source public under MIT by **21:30 IST** that day, within the
published one-hour window. To enter D, update the team form to select D after
its candidate verification; a new directory alone does not prove registration.

## Repository map

```text
track-a/                    Ingest: source, Dockerfile, metadata, README, PERF
track-c/                    Join: source, Dockerfile, metadata, README, PERF
track-b/                    Index: source, Dockerfile, metadata, README, PERF
track-d/                    Traverse: verified bidirectional search; registration pending
benchmarks/                 Recorded experiments, hashes and verification evidence
tests/                      Development fixture generation and coverage notes
tools/                      Differential runners, profiling and release checks
docs/assets/                Repository presentation assets
SUBMISSION_REPORT.md        Technical evidence and reproduction commands
SUBMISSION_CHECKLIST.md     Registration, app access and deadline checklist
SUBMISSION_DETAILS.txt      Values to copy into the team submission form
STATUS.md                   Current state and continuation notes
```

Generated datasets, development binaries and large local artifacts stay in
gitignored directories outside the track build contexts. The original verified
development history is preserved; later file-import commits are documented in
[the publication receipt](benchmarks/publication.json).

## Licence

Released under the **[MIT licence](LICENSE)**. The original starter copyright
notices are retained. See the track READMEs for assumptions, limitations and the
next experiments worth testing.
