# Track B — exact search in bounded phases

C++20, Linux/x86-64, AVX2/BMI2, standard library and POSIX interfaces only.
Read `/data/docs.txt` and `/data/queries.txt`; write exactly `/out/results.txt`.
Local paths use `--data DIR --out DIR`; `--workers N` overrides affinity-based
worker selection. Zero third-party runtime dependencies; MIT, see ../LICENSE.

Queries are interned into a compact exact vocabulary. Fingerprints reject
probes; complete zero-padded token bytes decide equality. The corpus is scanned
once, with bounded padded mappings and AVX2 ASCII classification. Only A–Z fold;
all non-ASCII bytes separate tokens. Unknown tokens retain their positions.

Ten phases bound peak index storage. Each phase uses word-aligned document
ranges claimed dynamically by one persistent worker team. Common terms chosen
by a runtime sample use bitmaps; other hits are counting-sorted into compact
16-bit runs within8192-document blocks. Sampling chooses representation, never
an answer. Frequencies and run membership are exact.

Phrases are matched inline from complete token-ID sequences around a sampled
rare anchor. Every term is checked, repeated positions are preserved, and each
phrase/document pair is counted once. Phrase results are merged in document
order. No raw-text re-tokenization or positional index is required afterward.

Boolean evaluation preserves strict left-to-right AND/OR/NOT semantics. Sparse
lists and bitmaps use exact merges, membership tests and word operations.
Per-phase frequencies prove some expression prefixes empty. AND and NOT keep
such a prefix empty; OR revives it only when its right term is present. The
evaluator can skip a guaranteed-empty result or discard a proven-empty prefix
before revival, while preserving every later operator in strict LTR order.

Each phase owns disjoint documents. Exact counts and64-bit id sums add across
phases; ascending ranges retain the ten smallest ids. Dense summaries use
popcounts and six bit-position masks. Original query order and duplicates are
preserved in final output. Every index and sample is built from current inputs
inside the timed process; no seed recognition or precomputed answers.

The digest-pinned GCC15/Debian13 image builds offline, statically links C++/GCC
support, and uses only the documented Intel instruction set. Current candidate
fixture, fresh full7, repeated full1 and exact-image sandbox evidence appears
in PERF.md. Local QEMU checks establish compatibility; the board establishes
native Intel performance. Empty-prefix simplification awaits its official trial;
the earlier rare-AND branch was rejected after a native score regression.

Next: profile token dictionary dependencies and phase/query overhead on the
scoring CPU. Preserve the current passing source before any redesign.
