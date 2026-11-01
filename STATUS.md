# Kryptonite — Speed Demon entry status

Updated 2026-11-01T14:15:50.866136+00:00.
Freeze: 2026-11-02 15:00 UTC / 20:30 IST; public-source deadline 21:30 IST.

- Entrant workspace: this directory; branch `speed-demon`, no remotes. MIT licence.
- Team confirmed: Kryptonite. Registration, entrant GitHub URL, Board App and team form state unknown.
- All three C++20 entries are implemented; official checkouts unchanged.
- A: six line-boundary workers selected after 1/2/4/6/12 sweep; full seed 1/7, fixtures and small sanitizer checks pass. Final public amd64 offline build and five sandbox runs pass. Official tooling locally measures S=187, declared D=1 (unverified).
- C: global exact name IDs and country bitsets kept (673bb07); full seeds 1/7, fixtures and small sanitizer checks pass. Final public amd64 offline build and five sandbox runs pass. S=218, declared D=1 (unverified).
- B: baselines preserved at a45c753, hybrid Boolean at f63225a, parallel indexing at e9fc252. B3 parallel queries kept at a3adf36. Worker comparisons 1/2/4/6/12 pass; six selected. Final-source native matrix is running serially; no further algorithm changes planned.
- Host ARM64 macOS; Apple Clang 17; Python 3.13.12. Docker Desktop engine running. Pinned GCC 15.3.0 and Debian 13 images prepared; final builds offline for linux/amd64 emulated on ARM64.
- No official T/M/K/points or board IDs exist. Local RSS is not cgroup M. Full official harness memory checks require Linux-host/Colima cgroup access unavailable on this Mac.

Remaining local work:
1. B3 tuning complete. Finish `tools/final_check.py` and inspect its retained evidence.
2. Run final-source tiny/sample seed 1, additional samples 2/3/42, full seed 7 and five full seed 1 runs for all tracks. Reuse valid evidence where source digest matches.
3. Final B amd64 offline build, five sandbox checks, official static S/D; representative full seed 7 containers for A/C/B.
4. Complete README/PERF and generate report with `python3.13 tools/report.py`. Validate report assertions, final runtime manifest and clean checkout builds. Commit only task-owned files.

Current command: `python3.13 tools/final_check.py > .work/final-native-progress.txt`. Updated 2026-11-01T14:21:50.711058+00:00.
Raw evidence: `benchmarks/track-*/`; large artifacts, inputs, outputs and binaries: ignored `.work/`.

2026-11-01T14:26:49.439169+00:00 — Final native matrix PASS for A/C/B. All final source digests pass required examples, targeted/sanitizer fixtures, tiny/sample/full seed 1, sample seeds 2/3/42, full seed 7 and five deterministic full runs. Next: archive committed source, clean offline amd64 builds/sandbox runs, final static tools and report.
