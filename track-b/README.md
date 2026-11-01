# Track B — query-driven exact search

C++20, standard library and POSIX mmap, zero third-party runtime dependencies.
Read `/data/docs.txt` and `/data/queries.txt`; write `/out/results.txt`.
Native testing supports `--data DIR --out DIR`. MIT; see ../LICENSE.

Queries are parsed first and their actual terms interned with exact equality.
Only those terms receive postings. Documents are scanned in ID order; the last
ID in each posting suppresses repeated occurrences within a document. All
postings stay sorted. Boolean intersections, unions and differences run strictly
left to right. An empty intermediate set can still grow at a later OR.

Phrases use the smallest posting as an anchor, check membership of all terms,
then tokenize each candidate document and check the exact contiguous sequence.
Unneeded tokens occupy sentinel positions and cannot create false adjacency.
Repeated terms require repeated positions; document boundaries never overlap.
ASCII A–Z alone are lowercased, ASCII letters/digits form tokens, every other
byte separates. Stopwords and zero-padded numbers retain their literal identity.

Sorted results give count, a 64-bit full ID sum, and the ten smallest IDs.
Input-backed keys and document views live as long as their mappings. Hash
collisions compare actual term bytes. All index work happens in the timed run.

Pinned allowlisted GCC 15 builds offline, with libstdc++/libgcc linked statically;
Debian 13 supplies glibc. See ../STATUS.md for final runtime and board evidence.
Declared D=1 is unverified by the harness; official S is unknown until measured.

With another 24 hours: profile build versus queries, compare dense bitmaps with
sparse postings, then measure parallel query evaluation. This initial baseline
uses one worker and keeps raw document views rather than a full positional index.
