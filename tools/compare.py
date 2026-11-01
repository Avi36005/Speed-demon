#!/usr/bin/env python3
"""Alternate preserved source and current source; hash every timed output."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import time
import uuid

SUB = Path(__file__).resolve().parents[1]


def sha(path):
    with open(path, 'rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('track', choices='acb')
    ap.add_argument('--baseline', required=True)
    ap.add_argument('--hypothesis', required=True)
    ap.add_argument('--repeats', type=int, default=5)
    ap.add_argument('--data', type=Path)
    ap.add_argument('--expected')
    ap.add_argument('--workers', type=int)
    args = ap.parse_args()
    work = SUB / '.work/compare' / (args.track + '-' + uuid.uuid4().hex[:10])
    work.mkdir(parents=True)
    data = (args.data or SUB.parent / f'data/track-{args.track}/full-seed1').resolve()
    manifest = json.loads((SUB.parent / 'docs/SHA256SUMS.json').read_text())['tracks'][args.track]
    expected = args.expected or manifest['expected_output_sha256'][0]
    original = subprocess.run(['git', '-C', str(SUB), 'show', f'{args.baseline}:track-{args.track}/main.cpp'],
                              capture_output=True, check=True).stdout
    current = (SUB / f'track-{args.track}/main.cpp').read_bytes()
    flags = ['-std=c++20', '-O3', '-DNDEBUG', '-pthread']
    record = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'track': args.track,
              'hypothesis': args.hypothesis, 'baseline_revision': args.baseline,
              'architecture': platform.machine(), 'platform': platform.platform(), 'official': False,
              'compiler': subprocess.run(['clang++', '--version'], capture_output=True, text=True).stdout,
              'flags': flags, 'data': str(data), 'inputs': {p.name: sha(p) for p in sorted(data.iterdir())},
              'expected_sha256': expected, 'board_run_id': None, 'decision': 'pending review',
              'candidate_workers': args.workers,
              'memory_method': 'wait4 ru_maxrss, local process RSS, not cgroup M',
              'artifact_dir': str(work.relative_to(SUB)), 'versions': {}, 'commands': []}
    try:
        for version, source in [('baseline', original), ('candidate', current)]:
            folder = work / version
            folder.mkdir()
            (folder / 'out').mkdir()
            (folder / 'main.cpp').write_bytes(source)
            command = ['clang++', *flags, str(folder / 'main.cpp'), '-o', str(folder / 'solution')]
            proc = subprocess.run(command, capture_output=True, text=True)
            record['commands'].append({'argv': command, 'exit': proc.returncode,
                                       'stdout': proc.stdout, 'stderr': proc.stderr})
            assert proc.returncode == 0, proc.stderr
            record['versions'][version] = {'source_sha256': hashlib.sha256(source).hexdigest(),
                                           'time_samples_s': [], 'rss_bytes': []}
        for iteration in range(args.repeats):
            for version in (['baseline', 'candidate'] if iteration % 2 == 0 else ['candidate', 'baseline']):
                folder = work / version
                command = [str(folder / 'solution'), '--data', str(data), '--out', str(folder / 'out')]
                if version == 'candidate' and args.workers is not None:
                    command += ['--workers', str(args.workers)]
                with open(folder / 'stdout', 'wb') as stdout, open(folder / 'stderr', 'wb') as stderr:
                    start = time.perf_counter()
                    proc = subprocess.Popen(command, stdout=stdout, stderr=stderr)
                    _, status, usage = os.wait4(proc.pid, 0)
                    elapsed = time.perf_counter() - start
                    proc.returncode = os.waitstatus_to_exitcode(status)
                record['commands'].append({'argv': command, 'exit': proc.returncode})
                assert proc.returncode == 0, (folder / 'stderr').read_text()
                digest = sha(folder / 'out' / manifest['output'])
                assert digest == expected, digest
                entry = record['versions'][version]
                entry['time_samples_s'].append(elapsed)
                entry['rss_bytes'].append(usage.ru_maxrss * (1 if sys.platform == 'darwin' else 1024))
                entry['output_sha256'] = digest
        for entry in record['versions'].values():
            samples = sorted(entry['time_samples_s'])
            middle = samples[1:-1] if len(samples) >= 5 else samples
            entry['middle_mean_s'] = sum(middle) / len(middle)
            entry['peak_rss_bytes'] = max(entry['rss_bytes'])
        record['passed'] = True
    except Exception as e:
        record['passed'] = False
        record['failure'] = str(e)
    (work / 'record.json').write_text(json.dumps(record, indent=2) + '\n')
    with (SUB / f'benchmarks/track-{args.track}/comparisons.jsonl').open('a') as f:
        f.write(json.dumps(record) + '\n')
    print(json.dumps({k: v for k, v in record.items() if k != 'commands'}, indent=2))
    return 0 if record['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
