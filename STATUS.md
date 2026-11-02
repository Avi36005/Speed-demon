# Kryptonite — active takeover checkpoint

Updated 2026-11-02 08:03 UTC. Target: **90 live overall machine points**, aiming for top three; neither achieved.
Freeze today2026-11-02 15:00UTC/20:30IST. Stop broad rewrites by13:00UTC and reserve final two hours.

## Current source and official evidence

Pushed head **2d48af07a22b4571b5c542b6c34eda36c61f7ff7**; watcher matches, queue empty.
All four registered and all gates pass. Receipt `2026-11-02T07:53:57.581616+00:00`:
`benchmarks/board/2026-11-02T075357.581616Z.json`.
Latest assembled-head overall **72.681974**; historical best-three **73.012154**.
Neither90 nor top three achieved. Best THREE geometric mean, never all four.

|Track|Latest points|T s|M bytes|S|Run|
|---|---:|---:|---:|---:|---|
|A|79.918502|0.026918|24375296|436|run-4faaad9a8812|
|B|65.641637|0.325392|46702592|793|run-650505a4fc9b|
|C|73.19028|0.044476|13066240|379|run-e18f2b09b560|
|D|54.283219|0.467529|272220160|245|run-c63203a09904|

New B rare-AND branch regressed to65.641637 from prior66.167402 (S793vs775),
so local B has been restored byte-for-byte to42317bd, pending a meaningful C
checkpoint push. Rejected commit/evidence retained. A/C/D unchanged from463.
Archive branches preserve earlier evidence; Claude stopped, single writer.
Normal tested speed-demon pushes authorized; no force/backdate/visibility action.

## Verification state and local execution path

Current hashes match Claude native x86 ASan/UBSan/small/generated-sample records in experiments.jsonl:
A22 passes, B30 passes, C many at12/33workers, D11. Reuse only exact source/input/options proof.
Exact-current A/C/D full7 passed under QEMU; prior B and rare-AND B both passed full7 at12/1.
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

## Rejected B rare-AND trial — native result observed

Current working B source is the isolated rare positive AND anchor candidate,
SHA3d26d55e05de1197e3bb92a08e4f5e00e042c39fea3cea7e5bcdae9441884990.
A/C/D remain byte-identical to463de7b. New no-OR branch avoids materializing a
large first set, while mixed OR queries retain exact original LTR evaluation.
All fixtures UBSan halt-on-error + C++ bounds assertions pass; new independent
rare-AND/NOT/OR-revival fixture agrees with untouched oracle. Five paired full1
runs match published hash; full7 workers12/1 pass. Exact image clean offline
build + five sandbox full1 + one full7 pass, S793/D1. Source/build/proof ledger:
`benchmarks/sparse-and-anchor-trial.json`. QEMU mean2.551391 ->2.498198s is only
screening; native result65.641637 at2d48af0/run-650505a4fc9b. Reject; prior B restored locally.

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

## Active C experiment

C record batch16 passed UBSan/bounds1/12, independently derived new fixture,
public full1 hashes, full7, exact offline image five full1 plus one full7.
QEMU0.413203 ->0.455003s regressed10%; native dependency latency differs, so
one explicit board trial is prepared without claiming a gain. Ledger:
benchmarks/c-record-batch16-trial.json. C selected only for this native trial;
B restored42317bd, A/D unchanged. Next commit/push, observe exact run, reject if
complete score axes regress. B coarse diagnostic source is separate and running.
