# Kryptonite — verified source and active native trial

Target90 live overall machine points/top three remains unmet. Verified036225c overall72.971055; historical best-three73.256138. The freeze evaluates the actual selected head.

|Track|Time s|M bytes|S|Machine points|Native run at036225c|
|---|---:|---:|---:|---:|---|
|A|0.026911|25014272|436|79.808946|run-5f9bb2ac6c4e|
|B|0.321271|46911488|782|65.806136|run-81df8b60b921|
|C|0.043319|12263424|353|73.983336|run-5390ab67f3f3|
|D|0.483486|271896576|245|53.924001|run-7b43f4f31e6e|

All four tracks are registered; all five native board gates pass. D is currently fourth. C name-local country membership improves its prior historical score to73.983336 and is retained. Small timing differences remain noisy. Receipt: benchmarks/board/2026-11-02T083443.958935Z.json.

Native CI41413513572 passed exact036225c B/C:16 B checks including14 ASan,9 C checks including8 ASan, fresh sample42 at12workers and full-public native diagnostic oracle checks. Source/build/input hashes, compiler flags, commands and outputs are saved in benchmarks/native-ci/41413513572. GCC13.3 on Intel Xeon6975P-C is development evidence, separate from the official13500/GCC15 score.

The newly selected A/B trial is pinned in benchmarks/compact-a-six-b-trial.json. A reduces append-chunk slack and scratch allocations and avoids quadratic all-equal selection. B tests a six-worker default while preserving CLI overrides. C/D remain unchanged. Both candidates pass relevant UBSan/bounds fixtures, independently oracle-checked cases, full7, repeated public hashes, exact offline amd64 images, five default sandbox runs and fresh full7 image checks. A passes1/12worker fresh checks; B6 is explicit in its records. A S441/B S783, D1 each.

QEMU screening A.355806 ->.349413s is inconclusive. B12-v6 QEMU2.350044 ->2.834975s regresses while worker-state memory falls; one native physical-core tradeoff trial will decide. No native gain is claimed before its actual commit/run. Reject B6 if complete score axes lose. The current A12/B6 native ASan job is pending.

A manual bounded CI workflow can also screen dependent exact-key prefetch on one native core using an isolated development copy. That candidate is unselected and cannot be credited as a scored improvement. Final container builds use only the source in each track folder.

Local QEMU cannot initialize ASan; native CI supplies that missing evidence. QEMU elapsed/cgroup values include emulation and are never substituted for official T/M. Unchanged source proof is reused only by matching hashes/options. Rejected rare-AND, hot-cache and C lookahead experiments remain recorded.

Stop broad rewrites13:00UTC; freeze15:00UTC/20:30IST. Final assembled committed-archive images, an additional unused seed, repeat determinism and watched-head verification remain due. MIT retained; private repository visibility must be resolved by16:00UTC/21:30IST using applicable authorization. No historical ZIP is called current.
