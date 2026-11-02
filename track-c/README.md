# Track C — exact hostile-CSV join

C++20, standard library, POSIX `mmap` and AVX2 intrinsics; zero third-party
dependencies. Reads `/data/records.csv` and `/data/regions.csv`, writes
`/out/aggregates.txt`. Local runs may pass `--data DIR --out DIR --workers N`.

## How it works

1. **Lookup.** `regions.csv` maps each region id to a dense country index; ids
   ascend, so the table is sized from the last line. Unknown or out-of-range
   ids go to `_UNMATCHED`. Countries are printed in code order, with
   `_UNMATCHED` last.
2. **Exact parallel record boundaries.** The frozen record grammar makes an
   LF followed by an ID digit, after an optional BOM, a record boundary:
   names contain no LF, and an embedded note LF is followed by a lowercase
   word or quote. Pieces use this rule and are claimed dynamically. The parser
   still treats embedded LFs and commas inside quoted fields as data.
3. **Parsing.** Each record skips an optional BOM at its start, parses `id`,
   `region_id` and the amount's integer part with SWAR digit arithmetic, and
   scans quoted fields for `"` with AVX2, counting `""` pairs. Amounts are
   integer cents. CRLF, LF and an unterminated final record are accepted.
4. **Exact distinct names.** Two names are equal after unescaping exactly
   when their raw quoted bytes are equal, because unescaping is injective. An
   empty unquoted name and `""` are both empty. Names are interned by their
   raw bytes in one shared lock-free table: a slot holds a 16-bit tag and a
   pointer to an arena record `{length, country bits, bytes}`, published with
   a release/acquire CAS. Full bytes decide equality even when hashes collide.
   Lookups are pipelined with prefetches for both record headers and key bytes. Each name's country words are aligned
   atomics, constructed before publication and retained until workers finish.
   Atomic first-set detection credits exactly one worker-local distinct count
   for each `(country, exact name)` pair, regardless of concurrent encounters.
5. **Aggregates.** Each worker keeps private per-group sums, min and max, and
   byte totals (raw length minus `""` pairs). These are combined at the end;
   distinct-name counts add because each pair was credited to exactly one worker.

See [PERF.md](PERF.md) for measurements.
