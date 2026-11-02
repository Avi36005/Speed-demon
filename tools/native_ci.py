#!/usr/bin/env python3
"""Bounded native x86 differential verification; never official score evidence."""
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys

SUB = Path(__file__).resolve().parents[1]
OUT = SUB / '.work/ci-evidence'
OUT.mkdir(parents=True, exist_ok=True)
tracks = os.environ.get('TRACKS', 'bc')
assert tracks in ('bc', 'ab', 'bcd', 'a', 'd', 'abcd')
b_workers = int(os.environ.get('B_WORKERS', '12'))
assert b_workers in (6, 12)
assert platform.machine() in ('x86_64', 'AMD64')


def capture(argv):
    p = subprocess.run(argv, capture_output=True, text=True, check=True)
    return p.stdout


def runtime_hashes():
    return {str(p.relative_to(SUB)): hashlib.sha256(p.read_bytes()).hexdigest()
            for t in 'abcd' for p in sorted((SUB / f'track-{t}').iterdir())
            if p.is_file() and p.name in ('main.cpp', 'Dockerfile', '.speed-demon.toml')}


initial = {str(p.relative_to(SUB)): len(p.read_text().splitlines())
           for p in (SUB / 'benchmarks').rglob('*.jsonl')}
record = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
          'official': False, 'method': __doc__, 'tracks': tracks,
          'workers': {t: b_workers if t == 'b' else 12 for t in tracks},
          'commit': capture(['git', 'rev-parse', 'HEAD']).strip(),
          'official_commit': capture(['git', '-C', str(SUB.parent / 'official/speed-demon-2026'), 'rev-parse', 'HEAD']).strip(),
          'runtime_hashes_before': runtime_hashes(), 'python': sys.version,
          'compiler': capture(['g++', '--version']), 'cpu': capture(['lscpu']),
          'platform': platform.platform(), 'run_id': os.environ.get('GITHUB_RUN_ID'),
          'commands': [], 'passed': False}
try:
    assert 'avx2' in record['cpu'] and 'bmi2' in record['cpu'], 'required CPU features unavailable'
    for track in tracks:
        argv = [sys.executable, str(SUB / 'tools/suite.py'), track, '--fixtures',
                '--seeds', '42', '--scale', 'sample', '--workers', str(record['workers'][track])]
        console = OUT / f'console-{track}.txt'
        with console.open('w') as stream:
            p = subprocess.run(argv, stdout=stream, stderr=subprocess.STDOUT)
        record['commands'].append({'argv': argv, 'exit': p.returncode, 'console': console.name})
        print(track, 'PASS' if p.returncode == 0 else 'FAIL', flush=True)
        if p.returncode:
            print(console.read_text()[-8000:])
            raise RuntimeError(f'track {track} verification failed')
    # A single-worker native diagnostic identifies CPU work without the
    # standard runner's oversubscribed waiting threads. These are development
    # phases; the official twelve-worker scoring CPU remains authoritative.
    for track in tracks:
        if track not in 'abc':
            continue
        data = SUB.parent / f'data/track-{track}/full-seed1'
        module = {'a': 'a_ingest', 'b': 'b_index', 'c': 'c_join'}[track]
        jobs = [[sys.executable, str(SUB.parent / f'official/speed-demon-2026/generators/{module}.py'),
                 '--seed', '1', '--scale', 'full', '--out', str(data)],
                [sys.executable, str(SUB / 'tools/phase_profile_linux.py'), track, '1', '--native']]
        for i, argv in enumerate(jobs):
            console = OUT / f'profile-{track}-{i}.txt'
            with console.open('w') as stream:
                p = subprocess.run(argv, stdout=stream, stderr=subprocess.STDOUT)
            record['commands'].append({'argv': argv, 'exit': p.returncode, 'console': console.name})
            if p.returncode:
                print(console.read_text()[-8000:])
                raise RuntimeError(f'track {track} native diagnostic failed')
    if os.environ.get('PAIR_B_KEYS') == 'true' and 'b' in tracks:
        argv = [sys.executable, str(SUB / 'tools/native_pair.py')]
        console = OUT / 'native-pair-console.txt'
        with console.open('w') as stream:
            p = subprocess.run(argv, stdout=stream, stderr=subprocess.STDOUT)
        record['commands'].append({'argv': argv, 'exit': p.returncode, 'console': console.name})
        if p.returncode:
            print(console.read_text()[-8000:])
            raise RuntimeError('native paired screening failed correctness')
    record['runtime_hashes_after'] = runtime_hashes()
    assert record['runtime_hashes_before'] == record['runtime_hashes_after']
    assert not capture(['git', '-C', str(SUB.parent / 'official/speed-demon-2026'), 'status', '--porcelain'])
    record['passed'] = True
except Exception as error:
    record['failure'] = str(error)
finally:
    for path in (SUB / 'benchmarks').rglob('*.jsonl'):
        relative = str(path.relative_to(SUB))
        new = path.read_text().splitlines()[initial.get(relative, 0):]
        if new:
            target = OUT / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text('\n'.join(new) + '\n')
    (OUT / 'summary.json').write_text(json.dumps(record, indent=2) + '\n')
raise SystemExit(0 if record['passed'] else 1)
