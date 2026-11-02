# Kryptonite — active checkpoint

Updated 2026-11-02 08:29 UTC. Target90 live overall machine points and top three remain unmet.
Freeze today15:00UTC/20:30IST. Stop broad rewrites13:00UTC; reserve final two hours.
Entrant https://github.com/Avi36005/Speed-demon, branch speed-demon. Claude stopped;
single writer. Tested ordinary pushes authorized; no force/backdated commits.

## Native board evidence

Pushed head ccc259f26ed733dedaa25ddaa57a35ee07ad4cf8. Latest receipt
benchmarks/board/2026-11-02T080605.562657Z.json; watcher matches, queue empty,
all four tracks registered, all five gates pass. Latest trial overall72.088040;
historical best-three73.012154. Select BEST THREE, never all four. Final head
must contain selected code; historical bests alone do not certify it.

| Track | Machine points | Time | cgroup peak | Real run at `ccc259f` |
| :--- | ---: | ---: | ---: | :--- |
| A | 79.856533 | 0.026894 s | 24,780,800 B | `run-af057be8bbd9` |
| B | 65.461918 | 0.333285 s | 46,641,152 B | `run-aba2bc28e2a3` |
| C | 71.662257 | 0.049922 s | 12,931,072 B | `run-b603168892dc` |
| D | 52.201648 | 0.566995 s | 271,425,536 B | `run-aa2034569555` |

Rejected B rare-AND2d48af0/run-650505a4fc9b:65.641637 versus parent66.167402;
restored42317bd before the next experiment. Rejected C batch16 lookahead
ccc259f/run-b603168892dc:71.662257 versus unchanged parent73.190280. Extra scan
and source penalty lose; stop this line. C baseline retained in463de7b.
Rejected sample hot256 B cache locally:2.193313 ->2.394439s QEMU, no native claim.

## Selected new B/C trial, ready to push

Proof ledger benchmarks/prefix-and-inline-bits-trial.json pins exact source hashes,
all checks and images. A/D are unchanged from463de7b.
B a8bde297...: sound per-phase empty-prefix propagation through LTR AND/OR/NOT;
discard only prefixes proven empty before OR revival. S782/D1, QEMU five-pair
1.914635 ->1.836980s. UBSan+bounds all fixtures1/12, expanded independent fixture
agrees with frozen oracle, full7 at1/12, exact offline image five full1/one full7 pass.
C b3d9888e...: country membership colocated with immutable arena names; atomic
first-set detection credits one local distinct count, removing name IDs and separate
country bitmaps. S353/D1. QEMU1.131957 ->.910426s;236,695,552 ->234,336,256B
include emulator/page cache, NOT official T/M. Header-comment-only change retained
identical compiled release binary. UBSan+bounds fixtures1/12, forced collisions at33,
country-word boundaries independently derived and oracle-checked, full7 at1/12,
exact offline image five full1/one full7 pass. Native effect UNKNOWN until observed.

## Native sanitizer and diagnostics path

Manual-only .github/workflows/native-correctness.yml prepares a standard bounded
Ubuntu x86 CI job, read-only repo permission, pinned official actions, frozen oracle
158894dcefde61f459c0adfae841900458dbeb7f. Default B/C fixtures with native ASan/UBSan,
sample42 at12, plus one-worker native full-public phase diagnostics. No auto push
trigger. Artifact new logs/source hashes retained; local QEMU ASan cannot initialize.
Next: commit/push verified B/C, dispatch this one job, observe exact board and CI IDs.
Do not claim native sanitizer passed until its artifact is retrieved and checked.

Default Docker amd64 translator stalls before main on this AVX2 source. Native ARM
QEMU10.2.3 Haswell works; metadata .work/qemu-runtime/metadata.json pins emulator
and libraries. No host/binfmt changes. ASan allocator fails before main under QEMU;
UBSan halt-on-error+bounds fresh checks pass. Reuse Claude native ASan proof only
for unchanged A/D or exact parent hashes, not new B/C.

## Release requirements and continuity

Read score-push master/plan/analysis and linked A/B/C/D/measurement briefs (done).
Do not modify frozen specs/oracles/generators/harness. Keep experiments append-only;
match claimed improvement to commit/run. Pending board and CI IDs must be saved.
No new ZIP/files handoff until target/actual outcome agreed; existing root ZIP old.
MIT retained; repo private at latest check. Public by16:00UTC/21:30IST requires
applicable visibility authorization. Final assembled committed-archive offline images,
multiple fresh seeds and branch/watch verification remain due before freeze.

Archive branches codex/pre-claude-evidence-20261102 at3577d79 and
codex/pre-final-claude-handoff-20261102 at263c0ec preserve older unpushed evidence.
Only speed-demon is the official watched branch. Source-identical T changes are
variation; no fake rank/top-three/90 claim. Docker environment uses isolated
.work/docker-public-config and unix:///Users/avinashgehi3/.docker/run/docker.sock.
Python required /opt/homebrew/bin/python3.13; system python3 is3.9 and lacks file_digest.
No subagents, goals or automations were created.

## 08:40UTC observed update

036225c7dd50f0eb1107ffee842feb15e0cafa3b passed all board gates. C won
73.983336 (.043319s,M12,263,424B,S353), run-5390ab67f3f3; keep layout.
B65.806136 (.321271s,M46,911,488B,S782), run-81df8b60b921; small/noisy,
provisionally keep sound prefix simplification. Latest head overall72.971055;
historical best-three73.256138. Target90/top three remain unmet. Receipt
benchmarks/board/2026-11-02T083443.958935Z.json.

Native x86 CI PASSED: https://github.com/Avi36005/Speed-demon/actions/runs/41413513572
exact036225c; artifact benchmarks/native-ci/41413513572. B16 exact-source
checks (14ASan), C9 (8ASan), fresh sample42 workers12 and full-public native
phase oracle checks pass. GCC13.3, Intel Xeon6975P-C; not official13500 timings.
B one-worker phases scan.756387s, BOOL.221617, metadata.008877, sample.012710,
query prep.006726, phrase serialization.013315. C parse/intern/aggregate.186653s.
Local QEMU ASan failure no longer blocks changed B/C native sanitizer proof.

Next candidates, still isolated: A7094ac6a... reduces chunks1024->256 entries,
right-sizes scratch from actual discovered IDs, reserves group results, and
bounds all-equal quickselect degeneration. UBSan/bounds1/12, new independent
packed-escape/all-equal oracle fixture, five public pair checks, full7, clean
image/public5/fresh1 pass; S441. QEMU.355806->.349413s inconclusive, memory
292,950,016->294,174,720 includes translator/pagecache, not predicted native M.
B six-worker default candidate preserves CLI overrides. UBSan fixtures6 pass;
paired12-v6 QEMU2.350044->2.834975s regresses while cgroup316,997,632->
311,226,368 falls. A single explicit native6P-core tradeoff trial will decide;
no speedup claimed. Fresh/offline proof currently running. Native CI gains
manual ab selection and explicit B6 option so changed A12/B6 can be verified
without repeating unchanged C. Next: finish proof, select/push A+B6 trial,
observe exact native metrics, dispatch bounded native ab CI, preserve C winner.
