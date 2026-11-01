# Speed Demon entry status

Started 2026-11-01 13:03 UTC (18:33 IST). Freeze: 2026-11-02 15:00 UTC.

- New entrant repository on `speed-demon`; no remote configured. MIT licence retained.
- Order: A baseline and evidence, C baseline and evidence, B baseline and evidence, measured tuning, final review.
- Host: Darwin ARM64; Apple Clang 17; Python 3.13.12.
- Docker engine initially stopped. Container, Linux amd64, official static S, cgroup memory, and board checks pending.
- Confirmed team name: Kryptonite. Entrant GitHub URL unknown. Registration/app/form/account state unknown.
- Official checkouts unchanged; public input provenance in `../docs/download-verification.json`.
- Track A exact public full hash, tiny/sample, worked/edge/targeted/empty checks passed. ASan/UBSan small checks passed. First timing attempt failed due to sandbox sysctl; rerunning with wait4.
- Track C public full/reference, tiny/sample and worked/rule/header-only checks pass; small sanitizer checks pass.
- Track A baseline preserved at 886e42b. Track C baseline being archived.
- Current work: Track B query-driven baseline and regression matrix. Docker engine started; pinned amd64 bases downloading.
- Next command: `python3.13 tools/dev.py a --data ../data/track-a/tiny-seed1 --label tiny-seed1`

All A/C/B baselines match published full hashes and untouched references.
A/C five-run amd64 emulated sandbox checks pass; builds 8.37/9.33 s, images ~30 MB.
Official cgroup memory/board evidence remains unavailable. B baseline is being archived.
Additional sample seeds 2/3/42 and full seed 7 for A/C running serially.
Current experiment: phase profiles to guide allocation and query representation changes.
