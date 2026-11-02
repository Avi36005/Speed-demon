# Kryptonite — final verified entry

**The88–90 target was not reached.** Observed restored-head live overall:
**73.099089**. Historical best-three overall: **73.461599**. Gap to88:
**14.900911** points. Final hidden-seed scoring and judging are unknown.

## Actual source and native result

Runtime/build inputs match `4fe9f66016d3d5d3f0a9fc1871a525e35d0c811a`. All four tracks are registered and
all five board gates pass. Later evidence-only commits preserve those hashes.

| Track | T s | M bytes | S | D | K | Points | Run |
|---|---:|---:|---:|---:|---:|---:|---|
|A|0.027281|23,199,744|443|1|39.693286|79.935853|run-2e4ae48aea8e|
|B|0.322503|42,356,736|791|1|20.898397|66.005649|run-55af2256e3e7|
|C|0.042777|12,386,304|358|1|30.242558|74.030926|run-f786a2d33c07|
|D|0.550116|132,751,360|273|1|12.754643|55.283415|run-782a80663d84|

[Immutable receipt](benchmarks/board/2026-11-02T141505.591838Z.json) · [Source manifest](benchmarks/final-assembly.json)

The board selects the best three machine scores. Historical per-track bests
are not substituted for the measured assembled-head score. Single native runs
show variation; no current rank or top-three result is claimed.

## Takeover comparison

|Track|Takeover T / M / S / K / points (`463de7b`)|Restored T / M / S / K / points (`4fe9f66`)|
|---|---|---|
|A|0.027029s / 24,428,544B / 436 / 39.562790 / 79.864346|0.027281s / 23,199,744B / 443 / 39.693286 / 79.935853|
|B|0.311542s / 46,931,968B / 775 / 21.054650 / 66.167402|0.322503s / 42,356,736B / 791 / 20.898397 / 66.005649|
|C|0.045172s / 13,053,952B / 379 / 28.874571 / 73.025777|0.042777s / 12,386,304B / 358 / 30.242558 / 74.030926|
|D|0.485033s / 272,519,168B / 245 / 11.956048 / 53.879383|0.550116s / 132,751,360B / 273 / 12.754643 / 55.283415|

Dependency axisD=1 throughout; these receipts expose no separateQ axis.
Takeover assembled-head overall72.804418; restored head73.099089. D's sparse
exact-distance tables cut memory from272MB to133MB; it reached59.454812 in a
source-identical run but remains the fourth track. A's compact allocation and
small-group selection and C's name-local membership were retained. B's idle-worker
handling and dead-buffer release were retained; its later redesign was rejected.

## Verification

[final-verification.json](benchmarks/final-verification.json) audits current runtime,
Dockerfile and metadata hashes against the scored source, native CI and clean images.
Selected A/B/D native ASan/UBSan and fresh sample42: CI41439655126. Selected C:
CI41456311760. Exact-source local UBSan/bounds and relevant1/12-worker checks,
public full and full seed7 results are retained in per-track records.

Selected clean committed images passed five deterministic runs each on unused
full seed20261102, plus public-data checks. Proof: committed-release-7b28c32.json
for A/B/D, committed-release-aeeee4e.json for C. The C proof is source-specific;
its archive's rejected B is not selected. Original B also passes the added
independent inline/phase fixture. Images build offline from pinned allowed bases
and pass library/entrypoint checks, nonroot/read-only/no-network/8GB constraints,
exact single outputs and empty filesystem diffs.

Local QEMU time/memory includes emulation and is not official Intel evidence.
Native CI uses GCC13/Xeon; the entry builds with pinned GCC15. Failed/rejected
experiments and raw samples remain saved. No official specs, generators,
references, timer or scoring accounting were changed.

## Final attempts and selection

The user requested continued work after the routine optimization cutoff. The
LTO/scheduling build trial passed all checks but lost native score and was reverted.
C's monotone bit check passed native CI and scoring and was retained. D's fused
lookup and A/C page-table-population tests did not improve local screening.
B's inline rare-query redesign was13% faster locally and passed native sanitizer
CI41458042088, yet scored64.438235 versus66.571274 for the prior source. It was
reverted. Correctness success never substitutes for a complete score improvement.

Measured B CPU time remains dominated by corpus scanning/dictionary lookup, with
Boolean work next. C remains dominated by parsing/interning/aggregation. The tested
changes did not deliver the large gains across three tracks needed for88 overall.

## Submission state

[Avi36005/Speed-demon](https://github.com/Avi36005/Speed-demon) is PUBLIC under MIT,
explicitly approved by the user and verified through GitHub. The Board App watches
`speed-demon`; A/B/C/D are registered. Code updates need pushes, not ZIP uploads or
another team-form response. Code freeze is November2,20:30IST; public-source
publication was completed before the saved21:30IST deadline. No historical project
ZIP is represented as a current verified submission. No runtime checks remain pending.
