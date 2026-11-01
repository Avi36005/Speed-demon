# Differential regression matrix

`fixtures.py` materializes development inputs in ignored `.work/fixtures/`.
It imports hand-worked bytes/answers and edge cases from the unchanged official
tests; the runner independently invokes the unchanged reference on those bytes.
Hand-computed expected bytes are also checked, separately from the reference.
Fixture construction never enters the submitted runtime/build contexts.

Track A covers the worked example, all official edge cases, n=1/2/100/101 nearest
ranks, repeated values/lines, 0/UINT32_MAX, 100/498/499/500/599 statuses, unsorted
hours/month rollover, prefix/case/125-byte endpoint ordering, and empty input.
The parallel version exercises whole-line cuts with twelve workers on small
fixtures and worker counts 1/2/4/6/12 on the full input.

Track C covers the worked example; both empty text encodings; trailing-space
names; doubled quotes; multiline notes; LF/CRLF; BOM after both terminators;
literal BOM bytes inside fields; leading/no BOM; an unterminated final record;
header-only input; holes/high unknown regions; missing countries; negative
fractions and min/max initialization. Generated seeds exercise large exact
name sets, bitset word boundaries and 64-bit sums. The concurrent-name-membership
fixture independently aggregates values before CSV encoding, forcing repeated
name/country pairs across parser boundaries. A one-root-bucket development copy
forces overflow and publication contention under sanitizers.

Track B reuses every hand-computed official tokenizer, Boolean, phrase, digit,
empty-document, top-ten and worked-example case. Additional fixtures cover
revival after an empty intermediate, duplicate queries, eight-token repeated
phrases, a real unindexed gap between phrase words, a 32-byte token and a
100,000-document result whose ID sum exceeds uint32. Parallel tests check all
supported worker counts and original query output order. The bitmap-summary
fixture checks all six bit-position boundaries, a partial final bitmap word,
wide sums and transitions between sparse and dense intermediate sets.

Track D covers every official hand case, directed/parallel/duplicate/zero/max
weights, empty-edge and isolated nodes, a first-meeting trap, duplicate queries
and an independent Floyd-Warshall random all-pairs oracle. Public/full7 and
worker sweeps exercise large graphs and touched-distance reuse.

Reproduce fixtures/sanitizers and three generated sample seeds:

```sh
python3.13 tools/suite.py a --fixtures --scale sample --seeds 2 3 42
python3.13 tools/suite.py c --fixtures --scale sample --seeds 2 3 42
python3.13 tools/suite.py b --fixtures --scale sample --seeds 2 3 42
python3.13 tools/suite.py d --fixtures --scale sample --seeds 2 3 42
```

Run from the submission directory. Sanitizer runs are never performance samples.
Exact input/output hashes, compiler/flags, worker counts and exit statuses are
stored under `benchmarks/track-X/`. `first-difference.json` is retained beside
the candidate/reference outputs for a mismatch. No oracle is modified.
