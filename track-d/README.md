# Track D — exact shortest paths

C++20, standard library and POSIX `mmap`; zero third-party dependencies. Reads
`/data/graph.txt` and `/data/queries.txt`, writes `/out/distances.txt`. Local
runs may pass `--data DIR --out DIR --workers N`.

## How it works

1. **Graph build.** Two parallel parses (SWAR numbers): the first counts out-
   and in-degrees atomically, the second claims CSR slots with atomic cursors
   and writes forward and reverse edges. Self-loops are dropped because they
   never shorten a path. Parallel and duplicate edges are kept, and Dijkstra
   uses the lightest. (An owner-range fill without atomics was faster locally
   but slower on the scoring box, so it was reverted.)
2. **Queries.** Workers claim queries dynamically and run exact bidirectional
   Dijkstra. Each frontier uses a binary heap with lazy deletion, and the
   smaller frontier expands. A meeting is only an upper bound; the search
   stops once the frontier minima cannot improve it. `s == t` is 0, a source
   with no out-edges or a target with no in-edges is `-1`.
3. **Memory.** Each worker keeps sparse open-addressed distance tables for the
   nodes its searches actually reach. Keys are exact node IDs; probing and
   growth preserve entries, and only occupied slots are reset. Values from
   2^32 − 2 upward (reachable only through near-maximal weights) go exactly
   into a small per-thread side map, so every comparison sees the full 64-bit
   value. Graph construction and all search state are built inside the timed run.

See [PERF.md](PERF.md) for measurements.
