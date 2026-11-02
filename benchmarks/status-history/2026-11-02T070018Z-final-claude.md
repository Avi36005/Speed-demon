# Kryptonite — current checkpoint

Updated 2026-11-02 04:50 UTC. Target requested: **92 overall live-board points**.
Not reached: 92 is above the board's best entry on every track (best A 88.3,
B 82.4, C 84.9, D 76.3 at 02:50 UTC). Previous status: `benchmarks/status-history/2026-11-01T200600Z.md`.

## How the score works (harness/scoring.py, frozen)

`K = (T_ref/T)^0.5 · min(M_ref/M, 50)^0.2 · min(S_ref/S, 1)^0.15 · (D_ref/D)^0.15`,
`points = 50·log10(K)`, overall = geometric mean of the best three registered
tracks. 92 points needs K ≈ 69 on every track. S is `scc` code lines after
`clang-format --style=LLVM` (80 columns), Dockerfile included; prose is free.
The board's `/out` is tmpfs, so the output file counts towards M.

## Live-board runs this session (full public seed 1, one run per push)

| Commit | A | B | C | D (unregistered) |
|---|---:|---:|---:|---:|
| `5c216d1` (previous best) | 65.83 | 49.57 | 57.29 | 46.04 |
| `b6fb4a7` A two-pass rewrite | 67.26 | 50.02 | 56.67 | 46.29 |
| `b1d2ca3` B and C rewrites | 69.41 | 63.93 | 69.12 | 45.14 |
| `71f59b0` A per-hour design | **74.61** | 63.87 | 69.06 | 51.71 |
| `a8f69d1` B prefetch, C tables, D 32-bit | 72.35 | 63.76 | 69.71 | **54.31** |
| `c624b2e` C bit prefetch, B flat queries | 72.83 | 65.10 | **70.85** | 54.29 |
| `35d1a42` A in-place, D owner fill | 74.21 | 63.58 | 70.60 | 51.32 |
| `3f87fd9` B counting-sort blocks | 73.41 | 65.95 | 70.81 | 51.80 |
| `e05f716` C 8-bit regions, D atomic fill restored | 72.66 | **66.35** | 68.33 | 53.45 |
| `0023e65` B 32-bit offsets, C branch-free hash | 72.58 | 66.04 | **71.45** | 53.72 |

Registered overall (A/B/C): **57.18 → ≈70.0** on the latest run `0023e65`
(≈70.7 using each track's best run). Identical code varies by up to ~15% in T between runs (for
example B at 0.248 s and 0.290 s), so single-run differences under about
2 points are not meaningful. Receipts: `benchmarks/board/`.

Latest axes at `0023e65`:

| Track | T (s) | M (bytes) | S | Points |
|---|---:|---:|---:|---:|
| A | 0.053169 | 29,184,000 | 338 | 72.58 |
| B | 0.245401 | 95,227,904 | 696 | 66.04 |
| C | 0.053050 | 12,742,656 | 371 | 71.45 |
| D | 0.496534 | 272,211,968 | 238 | 53.72 |

## Verification for every pushed change

- Official fixtures (worked examples, edge cases, targeted cases) under
  AddressSanitizer + UndefinedBehaviorSanitizer, at several worker counts
  (1, 3, 5, 7, 12, 33).
- Generated tiny and sample seeds compared with the untouched official
  reference; full seed 1 output compared with the published hash.
- The board's own correctness, timeout and sandbox gates passed on every run above.
- All four tracks also match the official references on an independently
  generated full seed 7 (`benchmarks/verification-2026-11-02.md`).

Measured and rejected this session: O_DIRECT output (no effect on tmpfs),
D owner-range CSR fill (slower on the box), B branch-free hit path and
small-token cache (no gain or slower), smaller B blocks, transparent huge
pages for D (slower), and a lower B bitmap threshold (neutral on K).

Local tooling notes: `tools/dev.py` accepts `SD_CXX=g++` (the container's
clang lacks ASan runtimes) and adds `-mavx2 -mbmi2` on x86-64. Run sanitizer
suites with `ASAN_OPTIONS=detect_leaks=0`: allocations are intentionally
released at process exit.

## Open items

- D is implemented and scored, but the board still shows it unregistered.
  Entering it needs the team form updated by a team member; it would only
  count if it beat the lowest of A/B/C.
- The repository must be public under MIT within the hour after the freeze
  (2026-11-02 15:00 UTC), and the final commit must be on `speed-demon` by then.
- Final scoring uses a private holdout seed; these are public-seed numbers.
