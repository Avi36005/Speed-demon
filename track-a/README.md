# Track A — exact log ingestion

C++20, standard library, POSIX `mmap` and AVX2 intrinsics; zero third-party
dependencies. Reads `/data/requests.log`, writes `/out/summary.txt`. Local runs
may pass `--data DIR --out DIR --workers N` (default: the CPUs in the affinity mask).

## How it works

1. **One parse.** Workers claim complete-line pieces. AVX2 LF masks identify
   line ends; fixed timestamp fields select the hour. Parsing from both ends
   finds the exact endpoint, error flag and SWAR-decoded latency.
2. **Compact records.** Each worker stores a 32-bit endpoint/error/latency
   record in private per-hour chunks. Values outside the 19-bit latency field
   use a following full-width escape, preserving every uint32 latency.
3. **Exact endpoints.** One shared table hashes the first 16 and last 8 bytes.
   A published entry owns its key and full tail; a fingerprint only rejects
   probes. Exact byte comparisons decide equality, including long paths,
   prefixes and case differences. The catalogue is discovered from the input.
4. **Exact percentiles.** Workers claim hours, count and scatter records by
   endpoint, and select nearest ranks `(p*n + 99) / 100`. Branch-free Lomuto
   partitions reuse the right partition after each selected rank. Counts,
   errors and compact percentile results determine each output slice's size.
5. **Ordered output.** One persistent worker team coordinates through barriers.
   Records are released before output pages are allocated. Exact byte offsets
   let workers format independent hours directly into the mapped output,
   using a two-digit table. Hour order and unsigned endpoint order are fixed.

Errors are statuses 500–599 (499 is not), counts and ppm use 64-bit
arithmetic, duplicate lines count twice, and an empty input gives an empty file.

## Memory

The parse retains compact per-hour records and one shared endpoint table.
Each selecting worker reuses an hour-sized latency buffer. Those records are
released before the roughly 10 MB public output is formatted; `/out` is tmpfs
on the board, so its pages count toward the official peak too.

See [PERF.md](PERF.md) for measurements.
