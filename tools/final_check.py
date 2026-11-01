#!/usr/bin/env python3
"""Fill gaps in the final-source native verification matrix, serially."""
import datetime
import hashlib
import json
import platform
import re
from pathlib import Path
import subprocess
import sys

SUB = Path(__file__).resolve().parents[1]
ROOT = SUB.parent
sys.path.insert(0, str(SUB / 'tests'))
import fixtures

MODULES = {'a': 'a_ingest', 'c': 'c_join', 'b': 'b_index', 'd': 'd_traverse'}


def main():
    commands = []
    passed = True
    sources = {}
    compiler = subprocess.run(['clang++', '--version'], capture_output=True, text=True,
                              check=True).stdout.strip()

    def run(command):
        command = [str(x) for x in command]
        proc = subprocess.run(command, capture_output=True, text=True)
        commands.append({'argv': command, 'exit': proc.returncode, 'stdout': proc.stdout, 'stderr': proc.stderr})
        if proc.returncode:
            raise RuntimeError(proc.stdout + proc.stderr)

    try:
        for track, module in MODULES.items():
            source = hashlib.sha256((SUB / f'track-{track}/main.cpp').read_bytes()).hexdigest()
            sources[track] = source
            default = re.search(r'size_t worker_count = (\d+);',
                                (SUB / f'track-{track}/main.cpp').read_text())
            default_workers = int(default[1]) if default else 1
            log = SUB / f'benchmarks/track-{track}/experiments.jsonl'

            def ensure(label, data, repeat=0, sanitize=False, workers=None):
                records = [json.loads(s) for s in log.read_text().splitlines()]
                inputs = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                          for p in sorted(Path(data).iterdir()) if p.is_file()}
                hand = SUB / '.work/fixture-answers' / track / label
                hand_hash = hashlib.sha256(hand.read_bytes()).hexdigest() if hand.exists() else None
                valid = [r for r in records if r.get('source_sha256') == source and
                         r.get('passed') and r['dataset'] == label and
                         r.get('inputs') == inputs and
                         r.get('workers') == (workers if workers is not None else default_workers) and
                         r.get('architecture') == platform.machine() and r.get('compiler') == compiler and
                         (hand_hash is None or r.get('hand_expected_sha256') == hand_hash) and
                         len(r.get('time_samples_s', [])) >= repeat and
                         (not repeat or ('-O3' in (r.get('flags') or []) and
                                         '-DNDEBUG' in (r.get('flags') or []))) and
                         (not sanitize or '-fsanitize=address,undefined' in (r.get('flags') or []))]
                if valid:
                    print(track, label, 'already verified for current source', flush=True)
                    return
                cmd = [sys.executable, SUB / 'tools/dev.py', track, '--data', data,
                       '--label', label, '--reuse-reference']
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
                       sanitize=label != 'wide-sum', workers=12 if default else None)
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
                            'passed': passed, 'sources': sources, 'compiler': compiler,
                            'commands': commands}) + '\n')
    return 0 if passed else 1


if __name__ == '__main__':
    sys.exit(main())
