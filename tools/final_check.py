#!/usr/bin/env python3
"""Fill gaps in the final-source native verification matrix, serially."""
import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import sys

SUB = Path(__file__).resolve().parents[1]
ROOT = SUB.parent
sys.path.insert(0, str(SUB / 'tests'))
import fixtures

MODULES = {'a': 'a_ingest', 'c': 'c_join', 'b': 'b_index'}


def main():
    commands = []
    passed = True

    def run(command):
        command = [str(x) for x in command]
        proc = subprocess.run(command, capture_output=True, text=True)
        commands.append({'argv': command, 'exit': proc.returncode, 'stdout': proc.stdout, 'stderr': proc.stderr})
        if proc.returncode:
            raise RuntimeError(proc.stdout + proc.stderr)

    try:
        for track, module in MODULES.items():
            source = hashlib.sha256((SUB / f'track-{track}/main.cpp').read_bytes()).hexdigest()
            log = SUB / f'benchmarks/track-{track}/experiments.jsonl'

            def ensure(label, data, repeat=0, sanitize=False, workers=None):
                records = [json.loads(s) for s in log.read_text().splitlines()]
                valid = [r for r in records if r.get('source_sha256') == source and
                         r.get('passed') and r['dataset'] == label and
                         len(r['time_samples_s']) >= repeat and
                         (not sanitize or '-fsanitize=address,undefined' in (r.get('flags') or []))]
                if valid:
                    print(track, label, 'already verified for current source', flush=True)
                    return
                cmd = [sys.executable, SUB / 'tools/dev.py', track, '--data', data, '--label', label]
                if repeat:
                    cmd += ['--repeat', repeat]
                if sanitize:
                    cmd += ['--sanitize']
                if workers is not None:
                    cmd += ['--workers', workers]
                run(cmd)
                print(track, label, 'PASS', flush=True)

            for label in getattr(fixtures, 'make_' + track)():
                ensure(label, SUB / '.work/fixtures' / track / label,
                       sanitize=label != 'wide-sum', workers=12 if track in 'ab' else None)
            for scale in ['tiny', 'sample']:
                ensure(f'{scale}-seed1', ROOT / f'data/track-{track}/{scale}-seed1')
            for seed in [2, 3, 42, 7]:
                scale = 'full' if seed == 7 else 'sample'
                label = f'{scale}-seed{seed}'
                data = SUB / f'.work/data/track-{track}' / label
                if not data.exists():
                    run([sys.executable, ROOT / f'official/speed-demon-2026/generators/{module}.py',
                         '--seed', seed, '--scale', scale, '--out', data])
                ensure(label, data)
            ensure('full-seed1', ROOT / f'data/track-{track}/full-seed1', repeat=5)
    except Exception as e:
        passed = False
        print('FAIL', str(e), flush=True)
    with (SUB / 'benchmarks/final-native.jsonl').open('a') as f:
        f.write(json.dumps({'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                            'passed': passed, 'commands': commands}) + '\n')
    return 0 if passed else 1


if __name__ == '__main__':
    sys.exit(main())
