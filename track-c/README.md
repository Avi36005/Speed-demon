# Track C — exact concurrent CSV join

C++20 and POSIX mmap, zero third-party runtime dependencies. Read
`/data/records.csv` and `/data/regions.csv`; write only `/out/aggregates.txt`.
Local paths: `--data DIR --out DIR`; workers: `--workers 1..12` (default six for the current scoring-box trial).
MIT; see ../LICENSE.

Each parser consumes complete CSV fields, scans literal quoted runs in bounded
bulk operations and decodes escaped quote pairs exactly. BOMs are skipped only
at record starts; embedded LFs/BOMs remain field bytes. Mixed LF/CRLF, empty
names, meaningful trailing spaces and an unterminated final record are handled.
Quote parity over the actual bytes locates chunk boundaries at LFs outside
quotes. Escaped pairs flip twice, so physical newlines alone never split records.

A dense region lookup maps known IDs to discovered country IDs and holes or
above-range IDs to `_UNMATCHED`. Each worker owns numeric country aggregates
with 64-bit counts/sums and integer cents, including negative fractions.

Decoded names are owned once in an exact table partitioned across independent
locks. Fingerprints only filter probes; complete string equality resolves
collisions. A name's country-membership bits are protected by its shard lock.
Exactly the first insertion of each name/country pair increments one worker's
private distinct count; joining and summing the private counts is therefore
exact across workers. Membership stride follows the actual country count.
No replicated name-pool translation or global distinct-count approximation is
used. All table construction and queries run inside the timed process.

GCC 15 and Debian 13 are pinned to allowed digests; builds are offline and C++
runtime libraries link statically. The current candidate passes native hostile
grammar/sanitizer/fresh-seed checks and three exact restricted amd64 public
container runs. These local runs are ARM-host emulation. Official T/M/S/K and
real run IDs are in PERF.md for each recorded commit.

The shared-table version is a measured memory/time tradeoff trial: it is slower
locally than replicated private tables but uses substantially less process RSS.
The real cgroup-memory score must decide whether it stays. Next: profile lock
traffic and numeric parsing, then select workers on the scoring machine.
