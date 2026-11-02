# Kryptonite — active takeover checkpoint

Updated 2026-11-02 07:00 UTC. Target: **90 live overall machine points**, aiming for top three; neither achieved.
Freeze today2026-11-02 15:00UTC/20:30IST. Stop broad rewrites by13:00UTC and reserve final two hours.

## Current source and official evidence

Entrant https://github.com/Avi36005/Speed-demon.git, branch `speed-demon`.
Current local/pushed head **463de7b14c484d6efd2762ee694c0dc6a2fd53eb**, runtime code **42317bd**.
User confirmed Claude stopped. All 19 newer Claude commits were fast-forwarded; none overwritten.
Current runtime files remain unchanged. Local archive branches preserve older unpushed evidence:
`codex/pre-claude-evidence-20261102` at3577d79; `codex/pre-final-claude-handoff-20261102` at263c0ec.

Latest assembled-head overall **72.804418**; historical best-three overall **72.995658**.
Raw receipt `benchmarks/board/2026-11-02T064445.120607Z.json` (all latest runs463de7b).
Watcher matches, no pending run in that receipt, every track passed all five gates.
**A/B/C/D are now all registered.** D entry is complete and scored; do not repeat stale unregistered claims.
Overall always selects the BEST THREE registered scores; never geometric-mean all four.

| Track | Latest points | T s | cgroup M bytes | S | Run |
|---|---:|---:|---:|---:|---|
| A | 79.864346 | 0.027029 | 24,428,544 | 436 | run-b6aeae4ae6b5 |
| B | 66.167402 | 0.311542 | 46,931,968 | 775 | run-6b5417c586c5 |
| C | 73.025777 | 0.045172 | 13,053,952 | 379 | run-723ade7b4bf4 |
| D | 53.879383 | 0.485033 | 272,519,168 | 245 | run-9076dddcddde |

User-reported historical rank40/41 of111 is not a current rank claim. Latest measured D is fourth.
User authorized tested normal pushes. No force pushes, fake/backdated commits, paid infrastructure,
form attestations or visibility changes. Repo private at last check; MIT notice retained.

## Verification state and local execution path

Current hashes match Claude native x86 ASan/UBSan/small/generated-sample records in experiments.jsonl:
A22 passes, B30 passes, C many at12/33workers, D11. Reuse only exact source/input/options proof.
No structured full-seed7 record matches the latest current source hashes, so fresh full7 remains needed.
Old report/round7 manifest and Mac timings describe historical source, not this Linux/x86 rewrite.

Default Docker amd64 translator stalls before main on this AVX2 program (release and sanitizer).
Link-order and missing A targeted-oracle issues in development helper were fixed and failures retained.
A release probe also had an incorrect expected-hash argument; it timed out before comparison. Subsequent
checks derive hashes from saved fixture/reference bytes. Never guess an expected hash.

Working fallback: **native ARM QEMU10.2.3, Haswell-noTSX CPU model**, in an unprivileged ARM Docker image.
The SAME old x86 binary that stalled under the default translator passes the worked bytes under QEMU.
Exact CURRENT463 A release now passed every fixture at12workers under QEMU. Artifact:
`.work/final-claude-a-qemu-release.stdout` and corresponding cached-runtime record.
QEMU setup `.work/qemu-runtime/metadata.json` pins emulator/library/ARM image digests. No privileged
binfmt registration or host settings changed. These are compatibility checks, never native Intel timings.

`tools/cached_runtime.py` development changes: --cxx-flag; static sanitizer archive group; explicit
oracle provenance when hand bytes absent; timeout stderr; --qemu path with read-only copied x86 libraries.
Use EXACT Docker CPU flags: A -mavx2; B/C -mavx2 -mbmi2; D no extra CPU flags.
Compiler stays pinned GCC15; final runtime stays pinned Debian13. QEMU dev image is not a submission input.

## Tested B rare-AND trial ready for native push

Current working B source is the isolated rare positive AND anchor candidate,
SHA3d26d55e05de1197e3bb92a08e4f5e00e042c39fea3cea7e5bcdae9441884990.
A/C/D remain byte-identical to463de7b. New no-OR branch avoids materializing a
large first set, while mixed OR queries retain exact original LTR evaluation.
All fixtures UBSan halt-on-error + C++ bounds assertions pass; new independent
rare-AND/NOT/OR-revival fixture agrees with untouched oracle. Five paired full1
runs match published hash; full7 workers12/1 pass. Exact image clean offline
build + five sandbox full1 + one full7 pass, S793/D1. Source/build/proof ledger:
`benchmarks/sparse-and-anchor-trial.json`. QEMU mean2.551391 ->2.498198s is only
screening; native score effect is pending. Next: commit/push and observe it.

Current A/C/D also passed exact-current independent full7 under QEMU. Matching
native parent ASan records remain preserved. Local ASan cannot initialize under
QEMU (allocator range error; reserved-VA probe also failed). Do not claim ASan
passed for new source. UBSan + bounds assertions provide fresh changed-branch
checks. Default translator stalls even before main; QEMU10.2.3 Haswell works.

Earlier Boolean-only-posting candidate passed correctness but QEMU timing was
inconclusive1.972606 ->1.983256s; keep unselected. Old phases4/ring16 candidates
are based on50a13a7 and must not be selected as current. Persistent teams were
already implemented by Claude; do not repeat them. B9b64023 observed66.489154
versus current66.167402: noisy small difference, not automatic rollback proof.

Development tools now support QEMU, corrected static sanitizer archive order,
explicit compiler flags, exact oracle provenance and local paired screening.
Exact-image QEMU checks run uid65534 directly and leave child wall time unknown;
Docker elapsed includes startup. The failed nested timer attempt is retained.
No official harness files/behavior were changed.

## Documentation and next actions

Read score-push master fully (done), linked B/C/D/measure briefs and current source (done); preserve
local master instructions and unchanged official specs/oracles. SCORE_PUSH_PLAN historical sections still
need a full non-truncated read if relying on them; calibration refs parsed in analysis JSON.
Root README, B README and submission report now describe the actual current designs/proof;
A/C track narratives still need refresh before final delivery.
Save real experiment and board evidence. No new files/ZIP handoff yet; old project ZIP is outdated.
Final recorded source public under MIT by21:30IST; visibility action still needs applicable authorization.

Docker environment: `DOCKER_CONFIG="$PWD/.work/docker-public-config"` and
`DOCKER_HOST=unix:///Users/avinashgehi3/.docker/run/docker.sock`; use authorized sandbox escalation.
Python3.13. Serialize builds/generation/performance. Keep commentary updates within60seconds.
Do not spawn agents unless explicitly authorized (none yet). Do not create goals/automations implicitly.

Prior Claude STATUS archived at `benchmarks/status-history/2026-11-02T070018Z-final-claude.md`.
