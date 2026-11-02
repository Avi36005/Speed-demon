# Kryptonite — final verified entry

**The 88–90 target was not reached.** Last-try assembled-head live overall:
**73.275742**; historical best-three **73.537261**. Gap to 88: **14.724258**.
Final hidden-seed scoring and judging remain unknown.

## Actual selected source and native results

Runtime/build inputs match `f982f448c5b8aeebc1f6ab6a7e8901a80b88b10a`. All four registered tracks pass every
native board gate. Later evidence-only commits preserve these exact hashes.

|Track|T s|M bytes|S|D|K|Points|Run|
|---|---:|---:|---:|---:|---:|---:|---|
|A|0.025882|23,801,856|443|1|40.543642|80.396138|run-569ef1eafbce|
|B|0.288839|49,983,488|791|1|21.526610|66.648782|run-a59dfa110601|
|C|0.046053|11,837,440|358|1|29.412470|73.426575|run-fd99c86dbdab|
|D|0.453138|131,862,528|273|1|14.072255|57.418185|run-31be93353a04|

[Receipt](benchmarks/board/2026-11-02T143516.564031Z.json) · [Source manifest](benchmarks/final-assembly.json)

The board selects the best three scores. Historical per-track bests are separate
from this assembled-head result. No current rank/top-three result is claimed.

## Last try

Six B phases replace ten, reducing repeated query passes and index rebuilds.
Native B time fell from.317447s at1c9adf7 to.288839s, while memory rose from
42,827,776B to49,983,488B. Full score66.648782 exceeds prior best66.571274 by
.077508point: a small noisy margin, not a decisive optimum. Native CI41462799819
passed exact-source sanitizers and fresh-reference checks, so six phases are retained.
A/C/D source is unchanged; their metric differences are run variation.

## Takeover comparison

|Track|Takeover T / M / S / K / points (`463de7b`)|Selected T / M / S / K / points (`f982f44`)|
|---|---|---|
|A|0.027029s / 24,428,544B / 436 / 39.562790 / 79.864346|0.025882s / 23,801,856B / 443 / 40.543642 / 80.396138|
|B|0.311542s / 46,931,968B / 775 / 21.054650 / 66.167402|0.288839s / 49,983,488B / 791 / 21.526610 / 66.648782|
|C|0.045172s / 13,053,952B / 379 / 28.874571 / 73.025777|0.046053s / 11,837,440B / 358 / 29.412470 / 73.426575|
|D|0.485033s / 272,519,168B / 245 / 11.956048 / 53.879383|0.453138s / 131,862,528B / 273 / 14.072255 / 57.418185|

D=1 throughout; these receipts expose no separateQ axis. The takeover head overall
was72.804418. C's exact interning/membership, A's compact allocation/small-group
selection and D's sparse exact-distance state are retained. D cut memory roughly
in half but remains fourth. B's later inline-query redesign and the compiler/lookup
trials that lost were reverted; all raw failures and measurements remain recorded.

## Verification

[final-verification.json](benchmarks/final-verification.json) maps selected source,
Dockerfiles and metadata hashes to native CI and clean committed image proof.
A/D native ASan/UBSan:41439655126. C:41456311760. Selected B:41462799819.
All relevant1/12-worker fixtures and fresh full-seed7 checks pass. Selected images
passed five deterministic runs each on unused full seed20261102 plus public input.
B clean archive proof:committed-release-f982f44.json; C:committed-release-aeeee4e.json;
A/D:committed-release-7b28c32.json. Rejected B from the C archive is not selected.

Images build offline from pinned allowed GCC15/Debian13 bases and pass runtime
library/entrypoint checks, nonroot/read-only/no-network/8GB constraints, exact
single outputs and empty filesystem diffs. Native CI uses GCC13/Xeon; ARM QEMU
checks are compatibility evidence and their time/memory is never substituted for
native Intel scoring. No runtime correctness or build gate remains pending.

## Publication and freeze

Repository https://github.com/Avi36005/Speed-demon is PUBLIC under MIT, approved
by the entrant and verified through GitHub. The Board App watches speed-demon;
A/B/C/D are registered. Code updates need pushes, not a ZIP upload or another
team-form response. Freeze is November 2, 20:30 IST. No further code changes are
planned. The original fully verified ten-phase fallback1c9adf7 is preserved.
No old project ZIP is called the current verified entry.
