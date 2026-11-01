# Track D — exact bidirectional shortest paths

C++20, standard containers and POSIX mmap; zero external runtime dependencies.
Read `/data/graph.txt` and `/data/queries.txt`; write only `/out/distances.txt`.
Local paths: `--data DIR --out DIR`. `--workers 1..12` selects independent query
workers; four is the provisional default. MIT; see ../LICENSE.

Two bounded parsing passes count and fill forward and reverse compressed sparse
row adjacency, avoiding temporary edge triples. Every directed edge survives,
including parallel copies, duplicates, self-loops and zero weights. Reverse
adjacency carries each original directed edge's own weight. Construction
buffers and the mapping are released before queries.

Each query uses exact forward/reverse Dijkstra with binary min-heaps and 64-bit
distances. Stale heap entries are discarded before using frontier minima. A
meeting supplies an upper bound; the search stops only when the valid frontier
minima cannot improve that bound. Frontier size chooses the next direction to
balance work even with a UINT32_MAX first edge. Empty frontiers are handled
soundly. Addition is guarded, and valid final answers fit signed 64 bits.

The empty path is checked before zero-degree rejection, so isolated s == t is
zero. No-out sources and no-in targets are rejected from the required CSR
offsets. Other unreachable cases use exact search, with no guessed geometry,
component labels or search caps. Each worker reuses only its own distance
arrays and touched-node lists; labels are reset before any search reuses them.
Queries read an immutable graph and write disjoint answer positions. Joining
before output preserves all duplicate queries and the original order.

The scalar baseline and unsuccessful experiments remain in history and PERF.md.
D3 passes hand/adversarial/random all-pairs oracle tests, small sanitizers,
independent sample/full seeds and repeated exact public output checks. Current
container and official score status is recorded in STATUS.md. The board has
measured earlier D0 runs but marks D unregistered; a directory alone does not
register it. No final holdout or judge result is claimed.

Next: profile graph construction versus search, then compare a full-width
integer radix heap and select worker count on the real scoring CPU. Avoid
heavy route preprocessing unless its construction cost pays across the queries.
