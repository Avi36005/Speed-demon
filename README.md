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

> **Verified and public under MIT.** All four registered tracks pass native board gates.
> Last-try B best: **66.649** · Observed assembled overall: **73.276** · Historical best-three: **73.537**.
> The requested 88–90 target remains unmet. Final hidden-seed scoring and judging are separate.

## The entry

| Track | Workload | Approach | Output |
| :--- | :--- | :--- | :--- |
| **A · Ingest** | 3.1 million HTTP request lines | Compact per-hour records and exact percentile selection | `summary.txt` |
| **B · Index** | 50,000 queries over 250,000 documents | Six-phase sparse runs, bitmaps and inline exact phrase matching | `results.txt` |
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

Measured runtime inputs at `f982f44`; later documentation commits preserve them.

| Track | Machine points | Time | cgroup peak | Native run |
| :--- | ---: | ---: | ---: | :--- |
| **A** | 80.396138 | 0.025882 s | 23,801,856 B | `run-569ef1eafbce` |
| **B** | 66.648782 | 0.288839 s | 49,983,488 B | `run-a59dfa110601` |
| **C** | 73.426575 | 0.046053 s | 11,837,440 B | `run-fd99c86dbdab` |
| **D** | 57.418185 | 0.453138 s | 131,862,528 B | `run-31be93353a04` |

Overall is the geometric mean of the best three registered machine scores.
These are single public-data runs with visible timing variation. Historical
bests are recorded separately from the assembled-head measurement.

[Immutable receipt](benchmarks/board/2026-11-02T143516.564031Z.json) ·
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
