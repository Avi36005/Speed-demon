# Track B — query-driven exact search

C++20, standard library, POSIX `mmap` and AVX2/BMI2 intrinsics; zero
third-party dependencies. Reads `/data/docs.txt` and `/data/queries.txt`, writes
`/out/results.txt`. Local runs may pass `--data DIR --out DIR --workers N`.

## How it works

1. **Query vocabulary.** Queries are parsed into flat term and operator
   arrays. Every distinct term is interned in a 2^18-slot table: 14-bit
   fingerprints filter probes, and a full 32-byte comparison of the
   zero-padded key decides. Terms longer than 32 bytes cannot match any token.
2. **One tokenizing pass.** Workers own disjoint document ranges that start at
   multiples of 64, so bitmap words are private. Each document is
   ASCII-lowercased and classified 32 bytes at a time with AVX2, giving one
   token bit per byte. Tokens are cut from those bit masks, including tokens
   that cross 64-byte blocks. Their hashes are computed first and their slots
   prefetched, then they are resolved. A per-term "last document" check counts
   each document once.
3. **Two representations.** A sample of 8,192 documents predicts the common
   terms (more than 1/40 of documents), which get bitmaps that are written
   during the scan. All other terms keep their hits per 8192-document block,
   counting-sorted by term into 16-bit document runs. A per-term run index
   lists the runs in document order. The prediction only chooses a layout;
   every answer is exact either way.
4. **Boolean queries** run strictly left to right. The running set is a sorted
   list or a bitmap, which stays a zero-copy view of a term's bitmap until it
   is first written. AND, OR and NOT use word operations, bit tests or
   branch-free merges. A small list ANDed or NOTed with a large one uses direct
   run membership tests. Bitmap results report the exact count, the id sum
   from six bit-position popcounts per word, and the ten smallest ids.
5. **Phrase queries.** The rarest term seeds the candidates, and every other
   term must be present (bit test or run search). A candidate is then verified
   exactly in its raw text: each occurrence of the rarest term is found with
   an AVX2 two-byte scan and checked as a full token, then its neighbouring
   tokens are read backwards and forwards with the same byte rules. Repeated
   terms, overlaps, sentence separators and non-ASCII bytes are handled exactly.

Tokenization follows the spec byte for byte: only `A`–`Z` are lowercased, and
every byte outside `[a-z0-9]` (including all bytes ≥ 0x80) is a separator.

See [PERF.md](PERF.md) for measurements.
