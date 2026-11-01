#!/usr/bin/env python3
"""Generate an evidence-linked local handoff. Never invent account/board state."""
import datetime
import hashlib
import json
from pathlib import Path
import subprocess

SUB = Path(__file__).resolve().parents[1]


def sha(path):
    with open(path, 'rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def rows(path):
    return [json.loads(s) for s in path.read_text().splitlines()] if path.exists() else []


def main():
    now = datetime.datetime.now(datetime.timezone.utc).isoformat()
    commit = subprocess.run(['git', '-C', str(SUB), 'rev-parse', 'HEAD'], capture_output=True,
                            text=True, check=True).stdout.strip()
    release = json.loads((SUB / '.work/release-selection.json').read_text())
    lines = ['# Kryptonite submission report', '', f'Evidence snapshot: {now}.', '',
             f'Base commit at generation: `{commit}` on `speed-demon`.',
             f'Clean archived build inputs: `{release["commit"]}`. The runtime manifest below',
             'pins the exact source/build/metadata bytes verified from that archive.',
             'Documentation/evidence may be committed after this snapshot; `git rev-parse HEAD`',
             'identifies the final local delivery commit. No remote is configured and nothing was pushed.', '',
             '## Implementation', '',
             'C++20 entries A, C and B are implemented. MIT licence and starter notices retained.',
             'All runtime code is in each independent track folder. Zero third-party runtime',
             'dependencies; team metadata is Kryptonite / C++. Development artifacts are gitignored.', '',
             'A uses whole-line private worker tables and exact latency sorting. C uses a scalar',
             'quote-aware CSV parser, integer cents, global exact name IDs and country bitsets.',
             'B uses query-only vocabulary, worker-local sorted postings, sparse/dense Boolean',
             'operations in input order, and exact rare-anchor phrase verification.', '',
             '## Correctness and packaging evidence', '',
             '| Track | Public full seed 1 SHA-256 | Additional full seed 7 SHA-256 |',
             '|---|---|---|']
    manifest = {'generated_utc': now, 'base_commit': commit,
                'verified_archive_commit': release['commit'], 'tracks': {}}
    summaries = {}
    for t in 'acb':
        folder = SUB / f'track-{t}'
        digest = sha(folder / 'main.cpp')
        experiments = rows(SUB / f'benchmarks/track-{t}/experiments.jsonl')
        containers = rows(SUB / f'benchmarks/track-{t}/containers.jsonl')
        static_file = SUB / f'benchmarks/track-{t}/static-final.json'
        static = json.loads(static_file.read_text()) if static_file.exists() else None
        successful = [r for r in experiments if r.get('passed')]
        full1 = [r for r in successful if r['dataset'] == 'full-seed1']
        full7 = [r for r in successful if r['dataset'] == 'full-seed7']
        assert full1 and full7, f'{t}: missing full seed evidence'
        final_containers = [r for r in containers if r.get('passed') and r['source_sha256'] == digest]
        final_public = [r for r in final_containers if len(r['runs']) >= 5 and
                        r['expected_sha256'] == full1[-1]['output_sha256']]
        assert final_public, f'{t}: final public container check missing'
        assert final_public[-1]['build_context'] == str(Path(release['directory']) / f'track-{t}')
        assert final_public[-1]['dockerfile_sha256'] == sha(folder / 'Dockerfile')
        assert any(r.get('source_sha256') == digest for r in full7) or any(
            r['expected_sha256'] == full7[-1]['output_sha256'] for r in final_containers), f'{t}: final full seed 7 missing'
        assert static is not None, f'{t}: static axes missing'
        final = [r for r in successful if r.get('source_sha256') == digest]
        assert all(r.get('passed') for r in experiments if r.get('source_sha256') == digest), f'{t}: current-source failure unresolved'
        for label in ['tiny-seed1', 'sample-seed1', 'sample-seed2', 'sample-seed3', 'sample-seed42', 'full-seed1']:
            assert any(r['dataset'] == label for r in final), (t, 'missing current source check', label)
        measured = [r for r in final if r['dataset'] == 'full-seed1' and len(r['time_samples_s']) >= 5]
        assert measured, f'{t}: final native five-run evidence missing'
        samples = sorted(measured[-1]['time_samples_s'])
        local_mean = sum(samples[1:-1]) / len(samples[1:-1])
        summaries[t] = {'static': static, 'container': final_public[-1], 'native': measured[-1],
                        'mean_s': local_mean, 'source': digest}
        manifest['tracks'][t] = {name: sha(folder / name) for name in ['main.cpp', 'Dockerfile', '.speed-demon.toml']}
        lines.append(f'| {t.upper()} | `{full1[-1]["output_sha256"]}` | `{full7[-1]["output_sha256"]}` |')
    lines += ['', 'Each current source passes worked/targeted fixtures, small ASan/UBSan checks,',
              'public tiny/sample/full inputs, three additional sample seeds (2/3/42),',
              'and an independently generated full seed 7. References and generators remain',
              'unchanged. Full public inputs were rehashed against the published manifest.',
              'A/B worker counts 1/2/4/6/12 produce identical full public outputs repeatedly.',
              'C is deliberately scalar; quote-aware chunk splitting is therefore unnecessary.', '',
              'Every final linux/amd64 image builds from its track alone with `--network=none`',
              'and passes five public-full runs under the unprivileged/read-only sandbox.',
              'The official process timer is compiled unchanged. Output names/hashes, exit 0,',
              'root filesystem diffs, final runtime libraries, image size and entrypoint are checked.',
              'Each final image also passes a full seed 7 sandbox run. The frozen official',
              'spec/reference/generator repository remains at commit',
              '`158894dcefde61f459c0adfae841900458dbeb7f`, with no working-tree changes.',
              'These are ARM-host emulated Linux checks, not scoring-machine runs.', '',
              '| Track | Final native mean (middle 3/5, s) | Local peak RSS (bytes) | Offline build (s) | Image bytes | Tool S | Tool D |',
              '|---|---:|---:|---:|---:|---:|---|']
    for t, summary in summaries.items():
        c, n, s = summary['container'], summary['native'], summary['static']
        lines.append(f'| {t.upper()} | {summary["mean_s"]:.6f} | {n["peak_rss_bytes"]:,} | {c["build_s"]:.3f} | {c["image_bytes"]:,} | {s["S_lines"]} | {s["D"]} ({s["D_basis"]}) |')
    lines += ['', 'Native results use Apple Clang 17 / ARM64 / C++20 O3 NDEBUG pthread.',
              'They are local spawn-to-exit measurements. RSS includes resident input mappings',
              'and is not cgroup memory. Scored containers use the pinned GCC 15.3.0 toolchain,',
              'with libstdc++/libgcc linked statically into Debian 13 runtime images.',
              'S/D were measured locally using the unchanged official formatter/count tools.',
              'D=1 is a declaration with an unverified basis, as reported by that tool.', '',
              '## Reproduction', '', 'Run from the submission directory with Python 3.11+:', '', '```sh',
              'python3.13 tools/suite.py a --fixtures --scale sample --seeds 2 3 42',
              'python3.13 tools/suite.py c --fixtures --scale sample --seeds 2 3 42',
              'python3.13 tools/suite.py b --fixtures --scale sample --seeds 2 3 42',
              'python3.13 tools/dev.py a --data ../data/track-a/full-seed1 --label full-seed1 --repeat 5',
              'python3.13 tools/dev.py c --data ../data/track-c/full-seed1 --label full-seed1 --repeat 5',
              'python3.13 tools/dev.py b --data ../data/track-b/full-seed1 --label full-seed1 --repeat 5',
              'python3.13 tools/suite.py a --scale full --seeds 7',
              'python3.13 tools/suite.py c --scale full --seeds 7',
              'python3.13 tools/suite.py b --scale full --seeds 7',
              'python3.13 tools/container_check.py a',
              'python3.13 tools/container_check.py c',
              'python3.13 tools/container_check.py b', '```', '',
              'Before offline builds, pull the exact GCC/Debian digests in each Dockerfile.',
              'The official tools image preparation command is in `../official/speed-demon-2026/harness/README.md`.',
              'Then measure each track with `static_axes.py --track-dir track-X --work .work/static`.',
              'Raw commands, statuses, hashes, profiles, alternating comparisons and deterministic',
              'run records are in `benchmarks/track-X/`. Large local artifacts are in `.work/`.', '',
              '## Remaining external proof and account actions', '',
              '- Official scoring-box T/M/K/judge scores and actual board run IDs: unknown; no submissions made.',
              '- The unmodified full scoring harness requires Linux-host/Colima cgroup access.',
              '  This Mac has no `/sys/fs/cgroup` host filesystem and no Colima; Docker Desktop',
              '  uses cgroup v2 inside its VM. Official cgroup M and native i5-13500 measurements remain outstanding.',
              '- Entrant GitHub repository URL, registration, team form and Board App installation: unknown.',
              '  Publishing, pushing and submitting forms require the user-owned destination and authorization.',
              '- After authorization, configure only the entrant remote and push `speed-demon`.',
              '  Confirm the board observed the intended commit before freeze, 2026-11-02 20:30 IST.',
              '  The recorded source must be public under MIT by 21:30 IST that day.', '',
              'Implementation and available local checks are complete. Submission readiness is',
              '**ready except for the named external account/board/scoring-machine actions and proof**.',
              'No official performance or prize outcome is claimed.', '',
              'Next useful experiments: profile B vocabulary lookup after parallel indexing,',
              'and compare A exact multi-selection against sorting on the real scoring CPU.',
              'Stop inconclusive paths; preserve the current verified revisions and final verification window.', '',
              '## Exact runtime contents', '', 'See [runtime-manifest.json](benchmarks/runtime-manifest.json)',
              'for SHA-256 of each main.cpp, Dockerfile and metadata file.']
    (SUB / 'benchmarks/runtime-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    (SUB / 'SUBMISSION_REPORT.md').write_text('\n'.join(lines) + '\n')
    print('Wrote SUBMISSION_REPORT.md and benchmarks/runtime-manifest.json')


if __name__ == '__main__':
    main()
