# Track A — exact log ingestion

C++20, standard library and POSIX mmap only. Read `/data/requests.log` and write
`/out/summary.txt`. Local checks may pass `--data DIR --out DIR`.

The program parses the documented timestamp fields directly, discovers endpoint
strings from the input, and stores unsigned 32-bit latencies in contiguous endpoint/hour slices. Twelve workers in the current scoring-box trial (override with `--workers 1..12`) parse independent
whole-line ranges into private count tables and row buffers. Prefix sums assign
disjoint slices, then workers scatter exact latencies into one contiguous array. Hour tasks then sort disjoint buckets and format independent strings in parallel.
The final write visits those strings chronologically. Each bucket yields the
three exact nearest ranks.
Errors are precisely 500–599; counts and ppm arithmetic use 64 bits. Endpoints
are sorted by ASCII bytes and hours emitted chronologically. Duplicate lines
are retained. Empty input produces a zero-byte file.

All input views refer to a read-only mapping whose lifetime encloses aggregation
and serialization. Standard hash tables resolve collisions by exact string
equality. No public-seed answer tables or endpoint catalogue is compiled in.

Build: the allowlisted GCC 15 image first instruments this C++20 program, then
runs it on a build-time `sample` generated with seed8675309 by the byte-identical
official `a_ingest.py`. The second compile uses that profile with coverage and
missing-profile warnings treated as errors. The generator SHA is
`d82b43b8f617016808f85cbe9be86af1fee8466296a0848ab8e6908ee3c4b0b4`.
Python and training data stay in build stages; only the C++ binary enters the
Debian 13 runtime. No inputs or answers are retained in the runtime image.

All base images are pinned to allowlisted digests. The build runs offline and
statically links libstdc++ and compiler support; glibc remains supplied by
Debian. A clean amd64 build from this directory and five restricted public-full
runs passed, together with independent full seed7 and tiny/sample1/2/3/42 checks
on the identical prototype build. These are compatibility/correctness checks;
emulated times do not predict the official scoring-box effect of PGO.

Dependencies: zero non-standard-library runtime dependencies (declared D=1;
the harness marks this declaration unverified). MIT, see repository LICENSE.
Native Mac timings are not scoring-box evidence. See PERF.md and ../STATUS.md.

With another 24 hours: compare exact multi-selection with full sorting and
measure the 1/2/4/6/12 worker sweep on the scoring CPU. The selected worker count
comes from native ARM64 measurements, so its x86 optimum remains unknown.
