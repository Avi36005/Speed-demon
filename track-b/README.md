# Track B — query-driven exact search

C++20, standard library and POSIX mmap, zero third-party runtime dependencies.
Read `/data/docs.txt` and `/data/queries.txt`; write `/out/results.txt`.
Native testing supports `--data DIR --out DIR`. MIT; see ../LICENSE.

Queries are parsed first and their actual terms interned in a compact probing
table. A fingerprint filters probes; complete byte equality resolves every
collision. Query-backed keys remain mapped for the whole run.
Only those terms are indexed. Documents are scanned in ID order; the last
seen ID suppresses repeated occurrences within a document. Worker ranges align
to bitmap words. A frequent private term promotes to a range bitmap and releases
its redundant posting list. Global frequencies select the final sparse/dense
layout. Ordered ranges merge into exact sorted postings or disjoint bitmap
words, and each private term buffer is released after its global merge.
Sparse lists and dense intermediate sets use exact membership tests or
bitwise operations as appropriate. Boolean intersections, unions and differences
run strictly left to right. An empty intermediate set can still grow at a later OR.

For each phrase, the term with the lowest actual document frequency anchors
the candidate scan. Its sorted posting or bitmap is enumerated exactly; each
candidate must contain all terms before exact token-sequence verification.
Unneeded tokens occupy sentinel positions and cannot create false adjacency.
Repeated terms require repeated positions; document boundaries never overlap.
On x86-64, bounded SSE2 blocks classify and ASCII-fold sixteen bytes at a
time; a scalar tail and ARM fallback preserve the same tokenizer. Maximal
tokens can cross blocks, including the 32-byte limit; no block reads past its
document view. ASCII A–Z alone are lowercased, ASCII letters/digits form tokens,
every other byte separates. Stopwords and zero-padded numbers retain their literal identity.

Sparse/phrase results accumulate exact counts and 64-bit ID sums. Dense final
bitmaps are summarized directly: popcounts give counts, six bit-position masks
sum IDs exactly, and only the ten smallest IDs are enumerated. The x86 summary
function enables POPCNT, available on the published Intel scoring CPU; ARM uses
its normal C++20 popcount implementation.
Input-backed keys and document views live as long as their mappings. Hash
collisions compare actual term bytes. All index work happens in the timed run.
Six workers build the index, then independently claim small query batches over
immutable indexes. Each query owns its temporary result and writes one dedicated
answer slot. Joining before serialization preserves the original query order.
`--workers 1..12` overrides the provisional default of six.

Pinned allowlisted GCC 15 builds offline, with libstdc++/libgcc linked statically;
Debian 13 supplies glibc. See ../STATUS.md for final runtime and board evidence.
Source counts are versioned in PERF.md and static-axis receipts; declared D=1
has an unverified basis.
The clean amd64 image passes five full public sandbox runs and full seed 7;
these are ARM-host emulation checks. The original baseline passed the official live board; updated candidate
measurements and checks appear in PERF.md.

With another 24 hours: profile vocabulary lookup and phrase re-tokenization,
compare direct token matching with ID lookup inside candidate documents, and
repeat worker selection on the scoring CPU. Raw document views keep the design
smaller than a full positional index. The x86 optimum remains unknown.
