# Kryptonite — four-track native checkpoint

Updated 2026-11-02 11:47 UTC.
Target90/top three unmet. Stop broad rewrites18:30IST (13:00UTC); freeze20:30IST.
Latest pushed b6ebbc890b712b40d155c824f16e09181d76d380 passes all five board gates,
all four registered. Latest trial overall72.622399; historical best-three73.343024.
B6 regressed64.548498 and was rejected; new selected B restores twelve workers.
A compact allocation is retained79.952250; C unchanged source observed74.215556.

|Track|Points|Time s|M bytes|Run at b6ebbc8|
|---|---:|---:|---:|---|
|A|79.95225|0.027008|23,781,376|run-3e475613b1fe|
|B|64.548498|0.369951|39,047,168|run-081abd790d21|
|C|74.215556|0.042425|12,247,040|run-bc13d0d4199a|
|D|54.227357|0.470085|272,011,264|run-74efb88d7bc1|

## Selected new all-four trial — next push

benchmarks/four-track-portfolio-trial.json pins exact hashes and proofs.
A b2470c2a... S443: sort <=24-value groups once for all exact percentile ranks.
B f1ed726d... S791: free dead query buffers; bounded spin plus atomic wait for
idle workers/main, explicit notify and original acquire/release publication.
C c18bd52a... S357: prefetch record header AND actual key line, explicitly
construct shared atomic slots. Exact key equality and name-local bits unchanged.
D055f90ac... S273: sparse private exact distance tables instead of two full-node
arrays per worker. Exact64-bit side maps, stale-heap checks and bounds preserved.
All D1. All four pass fixtures UBSan/bounds1/12, independently derived and
oracle-checked edge cases, full7, five public pair hashes, exact offline images
five full1/one full7. QEMU screens: A.404389->.393222; B1.335104->1.176699;
C.632634->.630418; D1.001046->.873029. Never official T/M. D local cgroup
349,085,696->234,524,672B includes emulator/page cache. Native effect pending.

Next: commit/push selected sources, dispatch native CI tracksabcd B12, fetch
exact board runs, keep/reject by complete axes. No scored gains claimed until
observed. Preserve A7094ac6 atb6ebbc8, B a8bde297 at036225c (old best9b64023),
C b3d9888e at036225c/b6ebbc8, D bb065b3e atb6ebbc8 for fallbacks.

## Native verification already completed

CI41413513572 passed036225c B/C native ASan/UBSan fixtures, sample42 at12 and
full-public phase checks. CI41415961602 passedb6ebbc8 A12/B6 native sanitizers,
sample42 and full-public diagnostics. Artifacts benchmarks/native-ci/<runID>.
Native dependent-key prefetch B experiment regressed1.274706->1.304224 and was
rejected. New all-four source native sanitizer matrix is still pending.

Local source compatibility uses QEMU10.2.3 Haswell; its ASan cannot initialize,
so do not substitute local UBSan for native ASan. GCC15 pinned Debian13 images
build offline; native CI GCC13.3 Xeon is distinct from official13500 performance.
No official specs, references, datasets or scoring/harness behavior modified.

## Final unused-seed preparation running

Session61253: tools/prepare_holdout.py seed20261102 generates untouched full
inputs/reference bytes for all four. This prepares ORACLES ONLY, never claims
entry passed. .work/final-holdout-preparation.stdout and
benchmarks/holdout-oracle-manifests/20261102.json show completion. After final
source selection, run exact committed-archive images against these inputs.
Do not tune performance on this seed. Preserve interrupted preparation state.

Keep records append-only and source-matched. Single writer; Claude stopped.
Tested ordinary speed-demon pushes authorized; no force/backdate/fake metrics.
Repo private, MIT retained. Public by21:30IST requires applicable authorization.
No current rank/90 claim, no new ZIP handoff. Project ZIP historical.
Reserve18:30–20:30IST for final source selection, clean committed-archive images,
unused-seed correctness, determinism, branch/watch/head/license verification.
Python3.13 required. Docker isolated .work/docker-public-config,
DOCKER_HOST unix:///Users/avinashgehi3/.docker/run/docker.sock. No agents/goals.
