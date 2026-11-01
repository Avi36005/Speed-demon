# Speed Demon entry status

Started 2026-11-01 13:03 UTC (18:33 IST). Freeze: 2026-11-02 15:00 UTC.

- New entrant repository on `speed-demon`; no remote configured. MIT licence retained.
- Order: A baseline and evidence, C baseline and evidence, B baseline and evidence, measured tuning, final review.
- Host: Darwin ARM64; Apple Clang 17; Python 3.13.12.
- Docker engine initially stopped. Container, Linux amd64, official static S, cgroup memory, and board checks pending.
- Confirmed team name: Kryptonite. Entrant GitHub URL unknown. Registration/app/form/account state unknown.
- Official checkouts unchanged; public input provenance in `../docs/download-verification.json`.
- Track A exact public full hash, tiny/sample, worked/edge/targeted/empty checks passed. ASan/UBSan small checks passed. First timing attempt failed due to sandbox sysctl; rerunning with wait4.
- Current work: Track C scalar parser baseline; Docker engine started, preparing pinned images.
- Next command: `python3.13 tools/dev.py a --data ../data/track-a/tiny-seed1 --label tiny-seed1`
