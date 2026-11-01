# Kryptonite — Speed Demon entry status

Updated 2026-11-01T14:39:13.939441+00:00. Local implementation and available verification COMPLETE.
Branch: `speed-demon`; remote `https://github.com/Avi36005/Speed-demon`.
Source push confirmed on 2026-11-01T14:39:13.939441+00:00; first acknowledged remote tip
`9ba13f4a2ca0b7693c9da1f5699c642126767fdd` contained 69 commits, including 56 nonempty file imports.
Original development history and actual timestamps are preserved. The import
commits use their real creation times. Runtime track files match verified
`90a353b` byte for byte. Repository visibility remains PRIVATE; default branch
is `speed-demon`. See `benchmarks/publication.json`. MIT licence.
Clean runtime/build inputs verified from commit `47bb5276e4296119b3774b564a72f13a3692ffd0`.
Later commits contain delivery documentation and evidence; exact runtime hashes
are pinned in `benchmarks/runtime-manifest.json`. Use `git rev-parse HEAD` for the
current delivery commit. All official checkouts remain unchanged.

## Final implementations

- A: six worker-local whole-line parsers, exact uint32 latency vectors, full per-group sorting. Baseline 886e42b, parallel version 2dab849.
- C: scalar quote-aware CSV, direct region join, exact global name IDs and country bitsets, integer cents. Baseline cc0f0e3, selected change 673bb07.
- B: query-term postings, sparse/dense Boolean operations, exact phrase verification, six workers for indexing and independent queries. Baselines a45c753 / f63225a / e9fc252, selected B3 a3adf36.
- All tracks declare team Kryptonite, language C++, zero non-stdlib runtime dependencies.

## Passed evidence

- Exact official worked/targeted fixtures and hand-computed bytes; small ASan/UBSan builds.
- Final-source tiny/sample/full seed 1, sample seeds 2/3/42 and independently generated full seed 7 against untouched references. Public full inputs match the published input hashes.
- Five repeated native full seed 1 runs per track; all outputs deterministic. A/B worker counts 1/2/4/6/12 checked repeatedly. C deliberately scalar.
- Clean archived offline no-cache linux/amd64 build for each independent track; allowlist checker, runtime libraries, entrypoint and image cap pass. Images about 30 MB.
- Five public full sandbox runs per image plus a full seed 7 run: unprivileged, read-only root/data, no network, 8 GB/no swap, PID/scratch limits, exact output names/hashes, exit 0, no unexpected filesystem diffs.
- Official static tools locally measure S: A=187, C=218, B=350; D=1 each, basis `declared, unverified`.
- Native ARM64 final middle-three means: A=0.250590 s, C=0.885086 s, B=2.230760 s. Local process RSS is recorded separately and is not official cgroup M.

## External proof/actions still pending

No official board run IDs, scoring-machine T/M/K, judge scores or submission are claimed.
This ARM64 Mac/Docker Desktop supports emulated sandbox/process-timer checks,
but the full official harness requires Linux-host/Colima cgroup access unavailable
here. Native i5-13500 performance and official cgroup memory remain outstanding.
Entrant repository: `Avi36005/Speed-demon` (private). Registration, team form and
Board App access remain unconfirmed. Source push has succeeded; changing
visibility, installing the app and submitting forms have not been requested.
Freeze: 2026-11-02 20:30 IST; recorded source must be public under MIT by 21:30 IST.

## Resume

Read `SUBMISSION_REPORT.md`, `benchmarks/DECISIONS.md` and per-track PERF.md.
Next local reproduction command: `python3.13 tools/final_check.py` (reuses matching
current-source proof); full commands are in the report. Source is pushed and the local branch tracks `origin/speed-demon`.
Next external steps: complete/confirm each member's registration, grant the Board
App access to `Avi36005/Speed-demon`, and submit Kryptonite / C++ / A+B+C / this
repo URL in the team form. Links and copyable values are in
`SUBMISSION_CHECKLIST.md`. The public board feed checked at
2026-11-01T14:38:06Z did not yet contain Kryptonite; that does not prove which
account step is missing. Make the recorded source public by the stated deadline.
No algorithm experiment is active. Future candidates: B phrase/vocabulary lookup
cost and A exact multi-selection on the real scoring CPU. Stop tuning paths that
produce inconclusive gains and retain a final verification window.
