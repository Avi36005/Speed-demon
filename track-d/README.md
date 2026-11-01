# Track D — exact directed shortest paths

C++20 with standard containers and POSIX mmap, no external runtime dependencies.
Read graph.txt and queries.txt under `/data`; write only `/out/distances.txt`.
Local paths can be supplied with `--data DIR --out DIR`.

The initial baseline parses the actual graph header, builds compressed sparse
row adjacency and runs forward Dijkstra with a standard binary min-heap for
each query. Stale entries are skipped; a reachable answer is returned only when
the target is settled. All edges, including parallel edges and duplicates, are
retained. Zero weights, directed edges and isolated nodes use ordinary Dijkstra
semantics. Distances use uint64 arithmetic with an overflow guard, and finite
answers fit signed 64 bits under the specification. Every query, including a
duplicate, gets a separate answer line in its original position.

All graph construction and search happens inside the measured process.
Construction buffers are released before queries. The initial version clears
its distance vector for each query; storage reuse, sound degree checks,
bidirectional search and parallel queries are candidates for measured tuning.

The pinned GCC 15 / Debian 13 Docker build follows the existing entry style.
MIT; see ../LICENSE. Verification and performance are recorded in PERF.md.
The scalar baseline passes local fixtures, independent sample seeds and five
full public runs. Container/fresh full-seed checks and registration remain
pending. This new track is not registered with the board. No D score is
claimed until a real board record is available.
