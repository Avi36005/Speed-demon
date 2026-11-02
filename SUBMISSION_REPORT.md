# Kryptonite — final verified entry

**90 was not reached.** Observed assembled-head live overall: **72.996661**.
Historical best-three overall: **73.431380**. Gap to90: **17.003339** points.
The live overall selects the best three registered machine scores. Final hidden-seed
measurements and judging remain unknown. No current rank or top-three result is claimed.

## Actual assembled-head result

Runtime commit `cda897a318b6cbe6b7d4751bc70d346fa4608480`. All four tracks registered; all five native board gates pass.

| Track | Points | T seconds | M bytes | S | D | K | Run |
|---|---:|---:|---:|---:|---:|---:|---|
| A | 80.194319 | 0.026335 | 23,875,584 | 443 | 1 | 40.168570 | `run-d94d9f16b2eb` |
| B | 66.205260 | 0.316628 | 42,373,120 | 791 | 1 | 21.091390 | `run-36f6fa75dbfb` |
| C | 73.261008 | 0.047442 | 11,489,280 | 355 | 1 | 29.189064 | `run-e41baccfb1c7` |
| D | 58.844665 | 0.395828 | 133,132,288 | 273 | 1 | 15.027727 | `run-a38947ec1bda` |

[Immutable receipt](benchmarks/board/2026-11-02T132724.472348Z.json) · [Exact source manifest](benchmarks/final-assembly.json)

## Takeover comparison

| Track | Takeover T / M / S / K / points (`463de7b`) | Final T / M / S / K / points (`cda897a`) |
|---|---|---|
| A | 0.027029s / 24,428,544B / 436 / 39.562790 / 79.864346 | 0.026335s / 23,875,584B / 443 / 40.168570 / 80.194319 |
| B | 0.311542s / 46,931,968B / 775 / 21.054650 / 66.167402 | 0.316628s / 42,373,120B / 791 / 21.091390 / 66.205260 |
| C | 0.045172s / 13,053,952B / 379 / 28.874571 / 73.025777 | 0.047442s / 11,489,280B / 355 / 29.189064 / 73.261008 |
| D | 0.485033s / 272,519,168B / 245 / 11.956048 / 53.879383 | 0.395828s / 133,132,288B / 273 / 15.027727 / 58.844665 |

D=1 throughout; no separate Q axis is present in these native receipts. The takeover
assembled-head overall was72.804418, compared with72.996661 here. Historical bests
are recorded separately. These are single-run measurements with visible variation.

A reduced allocation slack and avoided repeated small-group sorts. B releases dead
query buffers and parks idle workers, preserving strict LTR query semantics. C uses
name-local atomic membership and exact keys. D's sparse exact-distance tables cut
private memory substantially. D remains fourth in the live portfolio. B's native
single-worker diagnostic spent most time scanning/looking up corpus tokens; C's
main phase remains parsing/interning/aggregation. A90 overall needs much larger
speed gains across the selected tracks than the tested candidates delivered.

## Verification

The audit in [final-verification.json](benchmarks/final-verification.json) matches
runtime/build hashes to the scored source and native CI artifacts. Selected A/B/D
passed CI41439655126; selected C passed CI41449471565. This includes exact-source
ASan/UBSan fixtures and fresh sample42 at12workers. Relevant local UBSan/bounds
checks at1/12workers, public full and full-seed7 outputs are preserved in track logs.

Each selected image built offline from a clean committed archive with pinned allowed
bases, checked libraries and exact entrypoint. Five deterministic runs per image
passed unused full seed20261102, plus public-full checks. Runs use nonroot, read-only
root/data, no network,8GB/no swap, bounded tmp/processes, exact single output and
empty filesystem diffs. See committed-release-7b28c32.json and committed-release-1397636.json.

QEMU elapsed/cgroup values include emulation and do not replace native Intel metrics.
Native CI uses GCC13/Xeon; official entry builds use pinned GCC15. Rejected experiments,
failures, raw samples and exact source hashes remain in PERF.md and benchmarks.

## Submission state

The entry is pushed to `speed-demon`; the Board App watches it, and A/B/C/D are
registered. No ZIP upload or form resubmission is needed for code updates. Final
evidence-only commits preserve every verified runtime/build input above.

Code freeze: November2,20:30IST. Broad tuning stopped18:30IST. The repository is
private with MIT licensing. Explicit approval to make it public has been requested;
the saved public-source deadline is21:30IST. Do not treat private visibility as complete
submission compliance. No old project ZIP is presented as the current verified entry.

Publication update: the user approved public visibility on November2. GitHub
confirmed public/MIT and the git transport recovered after a transient visibility
propagation error. A final reversible build-only LTO/scheduling experiment is
running at the user's request for88+; it is not a claimed scored improvement.

Latest follow-up: the LTO2671675 trial regressed overall to72.396150 and all
added flags were rejected. A checked monotone C-membership candidate is now
awaiting native scoring, with original A/B/D and builds restored. Its local
proof is benchmarks/late-hotpath-trial.json. Native ASan and score remain
pending; the previous final verification describes the preserved fallback.
