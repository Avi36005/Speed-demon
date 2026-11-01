# Track B — query-driven exact search

C++20, standard library and POSIX mmap, zero third-party runtime dependencies.
Read `/data/docs.txt` and `/data/queries.txt`; write `/out/results.txt`.
Native testing supports `--data DIR --out DIR`. MIT; see ../LICENSE.

Queries are parsed first and their actual terms interned with exact equality.
Only those terms receive postings. Documents are scanned in ID order; the last
ID in each posting suppresses repeated occurrences within a document. All
postings stay sorted. Workers build private postings for ordered document-ID
ranges, then concatenate them in range order. Frequent terms also receive
bitmaps; sparse lists and dense intermediate sets use exact membership tests or
bitwise operations as appropriate. Boolean intersections, unions and differences
run strictly left to right. An empty intermediate set can still grow at a later OR.

Phrases use the smallest posting as an anchor, check membership of all terms,
then tokenize each candidate document and check the exact contiguous sequence.
Unneeded tokens occupy sentinel positions and cannot create false adjacency.
Repeated terms require repeated positions; document boundaries never overlap.
ASCII A–Z alone are lowercased, ASCII letters/digits form tokens, every other
byte separates. Stopwords and zero-padded numbers retain their literal identity.

Sorted results give count, a 64-bit full ID sum, and the ten smallest IDs.
Input-backed keys and document views live as long as their mappings. Hash
collisions compare actual term bytes. All index work happens in the timed run.
Six workers build the index, then independently claim small query batches over
immutable indexes. Each query owns its temporary result and writes one dedicated
answer slot. Joining before serialization preserves the original query order.
`--workers 1..12` overrides the provisional default of six.

Pinned allowlisted GCC 15 builds offline, with libstdc++/libgcc linked statically;
Debian 13 supplies glibc. See ../STATUS.md for final runtime and board evidence.
The official tooling locally measures S=350 and declared D=1 (unverified basis).
The clean amd64 image passes five full public sandbox runs and full seed 7;
these are ARM-host emulation checks. Official board performance is unknown.

With another 24 hours: profile vocabulary lookup and phrase re-tokenization,
compare direct token matching with ID lookup inside candidate documents, and
repeat worker selection on the scoring CPU. Raw document views keep the design
smaller than a full positional index. The x86 optimum remains unknown.
