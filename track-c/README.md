# Track C — exact hostile-CSV join

C++20, standard library, POSIX `mmap` and AVX2 intrinsics; zero third-party
dependencies. Reads `/data/records.csv` and `/data/regions.csv`, writes
`/out/aggregates.txt`. Local runs may pass `--data DIR --out DIR --workers N`.

## How it works

1. **Lookup.** `regions.csv` maps each region id to a dense country index; ids
   ascend, so the table is sized from the last line. Unknown or out-of-range
   ids go to `_UNMATCHED`. Countries are printed in code order, with
   `_UNMATCHED` last.
2. **Exact parallel record boundaries.** A byte is inside a quoted field
   exactly when an odd number of `"` precede it, because `""` pairs and field
   delimiters both contribute evenly. Workers count quotes in parallel. From
   the prefix parity, each chunk start scans to its first LF outside quotes,
   so embedded LFs, commas and quotes never cause a misplaced split.
3. **Parsing.** Each record skips an optional BOM at its start, parses `id`,
   `region_id` and the amount's integer part with SWAR digit arithmetic, and
   scans quoted fields for `"` with AVX2, counting `""` pairs. Amounts are
   integer cents. CRLF, LF and an unterminated final record are accepted.
4. **Exact distinct names.** Two names are equal after unescaping exactly
   when their raw quoted bytes are equal, because unescaping is injective. An
   empty unquoted name and `""` are both empty. Names are interned by their
   raw bytes in one shared lock-free table: a slot holds a 16-bit tag and a
   pointer to an arena record `{id, length, bytes}`, published with a
   release/acquire CAS. Lookups are pipelined (hash and prefetch eight
   records ahead), and each `(group, name)` bit is set in an atomic per-group
   bitset after its word has been prefetched.
5. **Aggregates.** Each worker keeps private per-group sums, min and max, and
   byte totals (raw length minus `""` pairs). These are combined at the end;
   `names_distinct` is the popcount of each group's bitset.

See [PERF.md](PERF.md) for measurements.
