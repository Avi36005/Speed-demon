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

Decoded names are copied into private parser arenas and published in one
shared exact table. Immutable entries and keys remain alive until all threads
join. Atomic bucket slots use release/acquire publication; full byte equality
resolves hash collisions. Buckets have monotonic overflow chains, so name count
is not capped by the initial root-table allocation.

Country-membership bits only change from zero to one. Exactly the first atomic
fetch-or for a name/country pair increments a worker's private distinct count;
joining and summing those counts is exact. Membership stride follows the actual
country count. No name pool, country catalogue or public answer table is
compiled in. Construction, parsing and lookup run inside the timed process.

GCC 15 and Debian 13 are pinned to allowed digests; builds are offline and C++
runtime libraries link statically. This arena/publication trial passes native
hostile grammar and full-seed7 checks, twelve-worker native and x86 sanitizer
fixtures, forced overflow contention, a clean offline amd64 build, five exact
public-full sandbox runs and independent full7. Docker runs here use ARM-host
emulation. Actual scoring-box effect remains pending; see PERF.md for real run
IDs and all earlier source versions.

Next: compare synchronization and numeric parsing costs on the scoring CPU.
Arena storage trades the original per-name allocation/destruction overhead for
bulk lifetime management. The previous sharded implementation remains in git
for direct time/memory/source comparisons and recovery.
