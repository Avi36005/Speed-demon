# Kryptonite — C native trial / verification checkpoint

Updated 2026-11-02T13:49:18.233708+00:00.
User requests88 minimum/90 preferred, still NOT achieved. User explicitly asked
continued improvement after earlier routine cutoff. Freeze15:00UTC/20:30IST;
preserve final verification time and original fallback8c5b801.

## Current working entry, next push

All Dockerfiles restored byte-for-byte8c5b801 after LTO2671675 lost. A/B/D
source unchanged8c5b801. ONLY C source changes: monotone country-membership
bits first relaxed-read, then fetch_or only if clear; first-set counting remains
exact under concurrency. Proof benchmarks/late-hotpath-trial.json pins source,
UBSan/bounds fixtures1/12, full7, offline image public5/unused20261102x5, S358/D1.
QEMU.390552->.350537s is screening only. Native result and native ASan pending.
Next: commit/push C+compiler rollback, dispatch workflow trackc, observe board
and exact CI IDs. Keep/revert based on native axes. Avoid new broad rewrites.

## Completed/rejected work

LTO2671675 full gates passed but points A79.857880/B64.092801/C74.134332/
D58.169270, overall72.396150. Reject all flags; restore original builds.
D fused sparse probe passed correctness but QEMU2.092769->2.336830s lost;
unselected, current D stays verified055f90ac. Prior sparse D gained~5points;
A compact allocation and small sorts gained~.4point. B remains limiting included
track. Do not claim88 or90. Latest verified fallback8c5b801 overall73.268128,
historical best-three73.431380. Final hidden-seed/judging unknown.

## Preserved verification

Original selected A/B/D native CI41439655126, old C CI41449471565 pass.
All original selected runtime/build hashes and clean committed archive images
match final-assembly.json; unused full seed20261102 FIVE deterministic runs plus
public checks pass. These certify fallback, not the changed C candidate.
C candidate local checks/images are source-matched in late-hotpath-trial.json;
new native source sanitizer proof must complete before final retention.
LTO exact images additionally passed40small fixtures and44full-image runs;
those rejected images are historical, not current source proof.

## Publication/remote

User explicitly approved PUBLIC visibility. GitHub confirms public,MIT,
disabled=false; transient git403 cleared, git transport works. Keep public.
Official branch speed-demon, all four registered, Board App watch okay.
No form update or ZIP upload required; no new files handed off before target.
No force/backdate/fake metrics, paid infrastructure, agents/goals/automations.
Python3.13, isolated Docker config .work/docker-public-config,
DOCKER_HOST unix:///Users/avinashgehi3/.docker/run/docker.sock.
