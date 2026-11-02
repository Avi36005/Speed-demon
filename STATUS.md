# Kryptonite — active refinement checkpoint

Updated2026-11-02 12:54 UTC.
Target90/top three remains unmet. Stop broad rewrites18:30IST/13:00UTC; freeze20:30IST.
Pushed7b28c326cf3c32f0862ca1e126cbdf31bae7e138 passes all native board gates,
all four registered. Actual latest head73.233294; historical best-three73.428587.
Receipt benchmarks/board/2026-11-02T115249.223434Z.json. Native points:
A80.232397 (run-6827b903f16b), B66.338782 (run-ea09eb10b8f4),
C73.791859 (run-0bb5937ce0ca), D58.982220 (run-adaac22899f6).
Sparse D memory131,268,608B vs272MB, runtime.393055 vs.47s: keep.
C key-line/new-array trial regressed; restore stronger winner, do not repeat.

## Current selected refinement, ready to push

benchmarks/portfolio-refine-trial.json pins all hashes/proofs. A unchanged b2470c2a.
B f64ad04b S798 packs unchanged slot/fingerprint into16-byte token records.
C7efd1e76 S355 restores winner; atomic_ref operates on proper uint64 slot objects.
Its GCC15 release binary EXACTLY matches old b3d9888e winner (hash in manifest).
D24b09441 S275 retains graph THP but uses small pages for private query tables.
All D1. All pass UBSan/bounds fixtures1/12, full7, five offline-image full1 plus
fresh7 image. QEMU means B1.275765->1.251320, C.294932->.286939,
D1.190711->1.200737 (screens only; no native gain claimed).
Next commit/push, native CI tracksbcd B12, observe exact native metrics. Preserve
A7b winner, Bf1ed726 at7b, Cb3d9888 at036/b6 (atomicref candidate same binary),
D055f90ac at7b as fallbacks. Smaller-distance frontier D candidate lost2.49x;
unselected. B inline-short/hot-key/dependent-prefetch variants rejected.

## Strong verification already complete

Native CI41439655126 PASSED7b all four: A7checks6ASan, B17checks15ASan,
C9checks8ASan, D11checks10ASan, fresh sample42 and native phase diagnostics.
Artifacts benchmarks/native-ci/41439655126. Compiler GCC13.3 Xeon; separate
from scoring13500/GCC15. Earlier CI41413513572 and41415961602 retained.
Clean COMMITTED archive7b images passed FIVE runs each on unused full seed
20261102 plus public1: benchmarks/committed-release-7b28c32.json COMPLETE.
The seed's generator/reference hashes and exact bytes are independently prepared
in benchmarks/holdout-oracle-manifests/20261102.json. It is for final correctness,
never performance tuning. Recheck changed refined images from committed archive.

Local QEMU Haswell compatibility, source-matched UBSan/bounds and exact images
are not native Intel timing/memory. New refined source native ASan pending.
No official specs/references/generators/harness were changed. Single writer.
User authorizes ordinary tested speed-demon pushes; no force/backdate/fake metrics.
Repo private, MIT retained. Public-source deadline21:30IST still needs applicable
authorization. No new ZIP handed off; old project ZIP is historical.
At18:30IST assemble strongest verified versions and reserve two final hours for
unused-seed/current-archive tests, deterministic output, metadata/watch/head checks.
Python3.13 explicit. Docker .work/docker-public-config and
unix:///Users/avinashgehi3/.docker/run/docker.sock. No agents/goals/automations.
