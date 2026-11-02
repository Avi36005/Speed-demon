#!/usr/bin/env python3
"""Native single-core B key-prefetch screening; never scoring-machine T/M."""
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import time
import uuid

SUB = Path(__file__).resolve().parents[1]
assert platform.machine() in ('x86_64', 'AMD64')
source = SUB / 'track-b/main.cpp'
original = source.read_bytes()
needle = '                for (size_t i = 0; i < n; ++i) {\n                    uint32_t id = pt[i].len'
replacement = '''                for (size_t i = 0; i < n; ++i) {
                    // The hash-slot pass has completed. Prefetch its dependent
                    // exact key eight tokens before the normal full comparison.
                    if (i + 8 < n && pt[i + 8].len <= 32) {
                        uint32_t e = *vocab.slot(pt[i + 8].h);
                        if (e != NONE) __builtin_prefetch(&vocab.keys[e & Vocabulary::MASK]);
                    }
                    uint32_t id = pt[i].len'''
assert original.decode().count(needle) == 1
candidate = original.decode().replace(needle, replacement).encode()
if 'static constexpr uint32_t IDMASK' in original.decode():
    # Inline short keys already avoid the dependent full-key load.
    candidate = candidate.replace(b'if (e != NONE) __builtin_prefetch(&vocab.keys[e & Vocabulary::MASK]);',
                                  b'if (e != NONE && !(e & Vocabulary::SHORT)) __builtin_prefetch(&vocab.keys[e & Vocabulary::IDMASK]);')
work = SUB / '.work/native-pair' / uuid.uuid4().hex[:10]
work.mkdir(parents=True)
data = SUB.parent / 'data/track-b/full-seed1'


def sha(path):
    with path.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


inputs = {p.name: sha(p) for p in data.iterdir() if p.is_file()}
expected = None
for raw in reversed((SUB / 'benchmarks/track-b/experiments.jsonl').read_text().splitlines()):
    prior = json.loads(raw)
    reference = SUB / prior['artifact_dir'] / 'reference/results.txt'
    oracle = SUB.parent / 'official/speed-demon-2026/reference/b_index.py'
    command = next((c for c in prior.get('commands', []) if c['exit'] == 0 and str(oracle) in c['argv']), None)
    if prior.get('passed') and prior.get('inputs') == inputs and command and reference.exists() and sha(reference) == prior['expected_sha256']:
        expected = sha(reference)
        provenance = {'prior_utc': prior['utc'], 'oracle_command': command, 'output': str(reference)}
        break
assert expected, 'matching untouched-reference bytes required'
cpu = min(os.sched_getaffinity(0))
flags = ['-std=c++20', '-O3', '-DNDEBUG', '-pthread', '-mavx2', '-mbmi2', '-static-libstdc++', '-static-libgcc']
record = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'official': False,
          'method': __doc__, 'hypothesis': 'Hide dependent exact-key latency after the existing hash-slot prefetch pass',
          'workers': 1, 'pinned_cpu': cpu, 'architecture': platform.machine(), 'inputs': inputs,
          'expected_sha256': expected, 'oracle_provenance': provenance, 'flags': flags,
          'compiler': subprocess.check_output(['g++', '--version'], text=True), 'versions': {},
          'artifact_dir': str(work.relative_to(SUB)), 'commands': [], 'decision': 'pending review'}
try:
    for name, code in [('baseline', original), ('candidate', candidate)]:
        folder = work / name
        folder.mkdir()
        (folder / 'out').mkdir()
        (folder / 'main.cpp').write_bytes(code)
        argv = ['g++', *flags, str(folder / 'main.cpp'), '-o', str(folder / 'solution')]
        p = subprocess.run(argv, capture_output=True, text=True)
        record['commands'].append({'argv': argv, 'exit': p.returncode, 'stdout': p.stdout, 'stderr': p.stderr})
        assert p.returncode == 0
        record['versions'][name] = {'source_sha256': sha(folder / 'main.cpp'), 'samples': []}
    for iteration in range(6):
        for name in (['baseline', 'candidate'] if iteration % 2 == 0 else ['candidate', 'baseline']):
            folder = work / name
            argv = ['taskset', '-c', str(cpu), str(folder / 'solution'), '--data', str(data),
                    '--out', str(folder / 'out'), '--workers', '1']
            with (folder / 'stdout').open('wb') as stdout, (folder / 'stderr').open('wb') as stderr:
                start = time.perf_counter_ns()
                p = subprocess.Popen(argv, stdout=stdout, stderr=stderr)
                _, status, usage = os.wait4(p.pid, 0)
                elapsed = time.perf_counter_ns() - start
                p.returncode = os.waitstatus_to_exitcode(status)
            got = sha(folder / 'out/results.txt')
            record['commands'].append({'argv': argv, 'exit': p.returncode})
            assert p.returncode == 0 and got == expected
            sample = {'wall_ns': elapsed, 'process_rss_bytes': usage.ru_maxrss * 1024,
                      'output_sha256': got, 'warmup': iteration == 0}
            record['versions'][name]['samples'].append(sample)
            print(name, iteration, elapsed, flush=True)
    for version in record['versions'].values():
        times = sorted(s['wall_ns'] / 1e9 for s in version['samples'] if not s['warmup'])
        version['middle_mean_s'] = sum(times[1:-1]) / 3
    assert source.read_bytes() == original
    record['passed'] = True
except Exception as error:
    record['passed'] = False
    record['failure'] = str(error)
(work / 'record.json').write_text(json.dumps(record, indent=2) + '\n')
with (SUB / 'benchmarks/track-b/native-comparisons.jsonl').open('a') as f:
    f.write(json.dumps(record) + '\n')
export = SUB / '.work/ci-evidence/native-pair'
export.mkdir(parents=True, exist_ok=True)
(export / 'baseline.cpp').write_bytes(original)
(export / 'candidate.cpp').write_bytes(candidate)
(export / 'record.json').write_text(json.dumps(record, indent=2) + '\n')
raise SystemExit(0 if record['passed'] else 1)
