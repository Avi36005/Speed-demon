# Track A — exact log ingestion

C++20, standard library, POSIX `mmap` and AVX2 intrinsics; zero third-party
dependencies. Reads `/data/requests.log`, writes `/out/summary.txt`. Local runs
may pass `--data DIR --out DIR --workers N` (default: the CPUs in the affinity mask).

## How it works

1. **Block hour summary (pass one).** The file is split at 16 KiB block
   boundaries, one range per worker. Each worker walks its lines (a newline
   search starting 39 bytes in, since lines are at least 40 bytes) and records,
   for every block, the set of hours that start a line there and the offset of
   the block's first line. Ranges start on block boundaries, so no block is
   shared between workers.
2. **One hour at a time (pass two).** Workers claim hours in order. For hour
   `h` a worker reads only the blocks whose summary contains `h` (3% more lines
   than the hour itself at seed 1) and skips other hours' lines. For each line
   of `h` it finds the method end with a word test, the endpoint end with two
   AVX2 compares, and parses the latency with SWAR arithmetic.
3. **Exact endpoints.** Each worker keeps an open-addressing table keyed by a
   hash of the endpoint's first 16 and last 8 bytes. A hit is confirmed by an
   exact byte comparison against a zero-padded 64-byte copy, and the tail of
   longer endpoints is compared against the input itself. Endpoints are never
   normalised; the 1,842-endpoint catalogue is discovered, not compiled in.
4. **Exact percentiles.** The hour's `(endpoint, latency)` rows are
   counting-sorted in place by endpoint (endpoints ordered by exact bytes, so
   prefixes sort first), and the three nearest ranks `(p*n + 99) / 100` are
   selected with a branch-free Lomuto quickselect. Each rank search reuses the
   right partition of the previous one.
5. **Ordered output.** Each finished hour is formatted with a two-digit lookup
   table. Whoever finishes an hour writes every consecutive finished hour, so
   workers never wait on each other to write.

Errors are statuses 500–599 (499 is not), counts and ppm use 64-bit
arithmetic, duplicate lines count twice, and an empty input gives an empty file.

## Memory

Only one hour's rows (at most 73k at seed 1) are held per worker, plus a small
endpoint table. There is no global latency array. The 10 MB output is the
largest single allocation, and it is charged to the run either way because
the board's `/out` is tmpfs.

See [PERF.md](PERF.md) for measurements.
