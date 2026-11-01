# Track A — exact log ingestion

C++20, standard library and POSIX mmap only. Read `/data/requests.log` and write
`/out/summary.txt`. Local checks may pass `--data DIR --out DIR`.

The program parses the documented timestamp fields directly, discovers endpoint
strings from the input, and retains unsigned 32-bit latencies in each endpoint's
72 hour buckets. Six workers (override with `--workers 1..12`) parse independent
whole-line ranges into private tables. Their exact vectors and error counts are
merged after joining all threads. It sorts every bucket and takes the three exact nearest ranks.
Errors are precisely 500–599; counts and ppm arithmetic use 64 bits. Endpoints
are sorted by ASCII bytes and hours emitted chronologically. Duplicate lines
are retained. Empty input produces a zero-byte file.

All input views refer to a read-only mapping whose lifetime encloses aggregation
and serialization. Standard hash tables resolve collisions by exact string
equality. No public-seed answer tables or endpoint catalogue is compiled in.

Build: `g++ -std=c++20 -O3 -DNDEBUG -pthread main.cpp -o solution`.
The offline Docker build statically links the GCC C++ and compiler support
libraries to avoid requiring the build compiler's libstdc++ in the runtime image;
glibc remains supplied by Debian 13. Final container compatibility must be tested.

Dependencies: zero non-standard-library runtime dependencies (declared D=1;
the harness marks this declaration unverified). MIT, see repository LICENSE.
Native Mac timings are not scoring-box evidence. See PERF.md and ../STATUS.md.

With another 24 hours: compare exact multi-selection with full sorting and
measure the 1/2/4/6/12 worker sweep on the scoring CPU. The selected worker count
comes from native ARM64 measurements, so its x86 optimum remains unknown.
