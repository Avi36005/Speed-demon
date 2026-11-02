<p align="center">
  <img src="docs/assets/kryptonite-banner.svg" alt="Kryptonite — Speed Demon 2026. C++20 entries for Ingest, Join, Index and Traverse." width="100%">
</p>

<p align="center">
  <strong>Kryptonite · Speed Demon 2026</strong><br>
  Four independent C++20 entries with exact outputs and reproducible builds.
</p>

<p align="center">
  <a href="#the-entry">Entry</a> ·
  <a href="#native-results">Results</a> ·
  <a href="#verification">Verification</a> ·
  <a href="#build-and-run">Build &amp; run</a> ·
  <a href="SUBMISSION_REPORT.md">Report</a>
</p>

---

> **Verified and public under MIT.** All four tracks are registered and pass native board gates.
> Observed assembled-head overall **73.099** · Historical best-three **73.462**.
> The requested 88–90 target remains unmet. Final hidden-seed scoring and judging are separate.

## The entry

| Track | Workload | Approach | Output |
| :--- | :--- | :--- | :--- |
| **A · Ingest** | 3.1 million HTTP request lines | Compact per-hour records and exact percentile selection | `summary.txt` |
| **B · Index** | 50,000 queries over 250,000 documents | Phased sparse runs, bitmaps and inline exact phrase matching | `results.txt` |
| **C · Join** | 3.32 million hostile CSV records | Exact name interning and name-local atomic country membership | `aggregates.txt` |
| **D · Traverse** | 2,000 directed shortest-path queries | Forward/reverse CSR and bidirectional Dijkstra with sparse state | `distances.txt` |

C++20 with the standard library, POSIX interfaces and supported x86 intrinsics.
**Zero third-party runtime dependencies.** Every track is an independent build
context. Parsing, indexing, graph construction and queries run on the actual
input inside the timed process. Hash fingerprints filter probes; full bytes
decide equality. Percentiles, integer cents, token positions and 64-bit distances
remain exact.

[Ingest design](track-a/README.md) · [Index design](track-b/README.md) ·
[Join design](track-c/README.md) · [Traverse design](track-d/README.md)

## Native results

Measured runtime inputs at `4fe9f66`; later documentation commits preserve them.

| Track | Machine points | Time | cgroup peak | Native run |
| :--- | ---: | ---: | ---: | :--- |
| **A** | 79.935853 | 0.027281 s | 23,199,744 B | `run-2e4ae48aea8e` |
| **B** | 66.005649 | 0.322503 s | 42,356,736 B | `run-55af2256e3e7` |
| **C** | 74.030926 | 0.042777 s | 12,386,304 B | `run-f786a2d33c07` |
| **D** | 55.283415 | 0.550116 s | 132,751,360 B | `run-782a80663d84` |

Overall is the geometric mean of the best three registered machine scores.
These are single public-data runs with visible timing variation. Historical
bests are recorded separately from the assembled-head measurement.

[Immutable receipt](benchmarks/board/2026-11-02T141505.591838Z.json) ·
[Kryptonite on the board](https://speeddemon.dev/board/#kryptonite-96429f)

## Verification

| Check | Verified scope |
| :--- | :--- |
| **Native correctness** | Every selected track passed ASan/UBSan fixtures and fresh sample 42 checks on x86 CI. |
| **Fresh full input** | Five deterministic exact runs per selected image passed unused full seed 20261102. |
| **Public data** | Published full hashes, repeated image runs and relevant worker/edge cases passed. |
| **Clean builds** | Images build offline from committed archives with digest-pinned allowed GCC 15/Debian 13 bases. |
| **Runtime limits** | Nonroot, read-only root/data, no network, 8 GB/no swap, bounded processes/tmp, exact single output and empty filesystem diffs passed. |

[Source manifest](benchmarks/final-assembly.json) · [Final audit](benchmarks/final-verification.json) ·
[Verification report](SUBMISSION_REPORT.md) · [Experiment records](benchmarks)

Native development CI uses GCC 13/Xeon; official performance uses the scoring
CPU and pinned GCC 15 builds. ARM QEMU checks establish compatibility; their
elapsed time and memory never replace native measurements. Per-track PERF logs
retain raw samples, rejected candidates and actual board run IDs.

## Build and run

Prepare the exact base-image digests in the chosen Dockerfile, then build from
that track alone. This example runs B with read-only inputs and a writable output:

```sh
docker build --platform linux/amd64 --network=none -t kryptonite-b track-b
mkdir -p out
docker run --rm --platform linux/amd64 --network=none --read-only \
  --memory=8g --memory-swap=8g --pids-limit=512 \
  --tmpfs /tmp:rw,size=1g,exec \
  -v "$PWD/data:/data:ro" -v "$PWD/out:/out:rw" kryptonite-b
```

The example expects B's `docs.txt` and `queries.txt` in `data`. Select another
track folder to build its independent image. Local path/worker overrides are
`--data DIR --out DIR --workers N`. Developer checks require Python 3.13 and the
unchanged official materials; see the report for source-matched reproduction.

## Submission

Repository: [Avi36005/Speed-demon](https://github.com/Avi36005/Speed-demon) ·
Watched branch: [`speed-demon`](https://github.com/Avi36005/Speed-demon/tree/speed-demon) ·
Licence: [MIT](LICENSE)

The Board App is connected and A/B/C/D are registered. Pushes to the watched
branch update the entry; a ZIP upload is unnecessary. A form change is needed
only when changing the registered repository or tracks. Freeze: **November 2,
20:30 IST**. The repository was made public with the entrant's approval.
