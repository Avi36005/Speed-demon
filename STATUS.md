# Kryptonite — active checkpoint

Updated 2026-11-02 08:49 UTC.
Target90/top three unmet. Freeze15:00UTC/20:30IST; stop broad rewrites13:00UTC.
Current pushed036225c7dd50f0eb1107ffee842feb15e0cafa3b; all four registered, all
board gates pass. Latest assembled-head72.971055; historical best-three73.256138.
Receipt benchmarks/board/2026-11-02T083443.958935Z.json. C73.983336 winner retained.

| Track | Machine points | Time | cgroup peak | Real run at `036225c` |
| :--- | ---: | ---: | ---: | :--- |
| A | 79.808946 | 0.026911 s | 25,014,272 B | `run-5f9bb2ac6c4e` |
| B | 65.806136 | 0.321271 s | 46,911,488 B | `run-81df8b60b921` |
| C | 73.983336 | 0.043319 s | 12,263,424 B | `run-5390ab67f3f3` |
| D | 53.924001 | 0.483486 s | 271,896,576 B | `run-7b43f4f31e6e` |

## Selected new A/B native trial — ready to push

Exact source/build/check/image hashes: benchmarks/compact-a-six-b-trial.json.
A compact256-entry chunks, right-sized scratch, group reserve and equal-value
quickselect guard. S441; fixture UBSan/bounds1/12, new independent oracle,
five public pairs, full7 at1/12, exact offline image five full1/one full7 pass.
B default six workers, CLI override intact. Source logic otherwise036 prefix.
UBSan/bounds6, full7 at6, exact offline image five full1/one full7 pass. QEMU6
slower20%; one native6P-core memory/time tradeoff trial only. Restore036 default
if complete axes regress. Do not claim gain before exact commit/run.
C/D unchanged. Next: push A/B trial, observe actual native metrics, dispatch
manual native CI tracksab,b_workers6,pair_b_keystrue. Prepared optional single-core
native B dependent-key prefetch experiment changes only a development copy;
its candidate is unselected. Preserve current runtime while remote tests run.

## Verification and native diagnostics

Native x86 CI41413513572 PASSED exact036225c B/C, artifacts
benchmarks/native-ci/41413513572. B16 exact-source checks14ASan, C9 with8ASan,
fresh sample42 at12 and full-public diagnostic oracles pass. Compiler GCC13.3,
Intel Xeon6975P-C; not scoring13500 timings. B single-worker scan.756387s,
BOOL.221617s, metadata.008877s; C parse/intern/aggregate.186653s. Main remaining
B cost is scan/lookup. Local QEMU cannot initialize ASan, so new A/B source
native ASan is pending. Local compatibility uses native ARM QEMU10.2.3 Haswell,
pinned metadata .work/qemu-runtime/metadata.json; never substitute its T/M.

## Rejected experiments and safeguards

B rare-AND2d48af0 scored65.641637: reverted. B sample-hot256 locally regressed:
unselected. C batch16 lookaheadccc259f scored71.662257: reverted. C current
name-local bits036225c improved73.983336: keep. Preserve all raw evidence.
Single writer; Claude stopped. Ordinary tested pushes authorized, no force,
backdating, manufactured commits/metrics or public visibility action. Repo
private last checked; MIT retained. Public-source deadline16:00UTC/21:30IST
still needs applicable authorization. No current rank/90/top-three claim.

Reserve final two hours for assembled committed-archive offline builds,
additional unused fresh seed, deterministic output and watcher/head validation.
No new ZIP handed off; project root ZIP is historical. Use Python3.13 explicitly;
systempython3 is3.9. Docker uses isolated .work/docker-public-config and
unix:///Users/avinashgehi3/.docker/run/docker.sock. No agents/goals/automations.
Prior detailed checkpoint archived in benchmarks/status-history/.
