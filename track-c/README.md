# Track C — exact CSV join

C++20 with zero third-party runtime dependencies. Read `/data/records.csv` and
`/data/regions.csv`; write `/out/aggregates.txt`. Native checks support
`--data DIR --out DIR`. MIT; see ../LICENSE.

The scalar parser uses bounded bulk scans over literal quoted runs and consumes
a whole quoted field before interpreting record terminators. Escaped quote pairs count as one decoded byte. A BOM is skipped
only at the start of a record, so embedded BOMs and LFs remain field data.
Both LF and CRLF and an unterminated final record are supported. Name strings
retain trailing spaces; the two empty encodings decode to the same string.
Notes are counted while scanning without materializing their text.

A dense uint16 region table maps known IDs to dynamically discovered country
IDs. Holes and above-range IDs map to `_UNMATCHED`. Decoded names are interned
globally in a compact probing table with exact string equality. Only unique
names are copied; repeated plain names use transient input views. Each country uses a dynamically sized bitset
of those IDs to count distinct names, plus 64-bit totals and min/max initialized
from integer extremes. The bitsets represent exact name IDs rather than hashes.
Money uses integer cents throughout, including negative fractional amounts.
Hash collisions use exact string equality. Countries without records are omitted.

The Dockerfile uses pinned allowlisted GCC 15 and Debian 13, statically linking
libstdc++ and libgcc for runtime compatibility. Final container checks and
official scoring are tracked in ../STATUS.md. The harness treats the declared
D=1 as unverified; the official tooling locally measures S=218. Clean amd64
emulated sandbox checks pass full seeds 1 and 7. The original baseline passed the official live board; new candidate checks
and measurements appear in PERF.md.

With another 24 hours: profile the remaining scanning and hashing costs, then
measure parallel parsing with proven quote-aware record boundaries. The selected
implementation uses one worker and no chunk splits, so there is no assumption
that physical newlines identify records.
