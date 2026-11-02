#!/usr/bin/env python3
"""Prepare independent full-seed reference bytes; does not test an entrant."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import sys

SUB = Path(__file__).resolve().parents[1]
OFFICIAL = SUB.parent / 'official/speed-demon-2026'
MODULE = {'a': ('a_ingest', 'summary.txt'), 'b': ('b_index', 'results.txt'),
          'c': ('c_join', 'aggregates.txt'), 'd': ('d_traverse', 'distances.txt')}
ap = argparse.ArgumentParser(description=__doc__)
ap.add_argument('--seed', type=int, default=20261102)
args = ap.parse_args()
root = SUB / '.work/holdout-oracles' / str(args.seed)
root.mkdir(parents=True, exist_ok=True)
evidence = SUB / 'benchmarks/holdout-oracle-manifests'
evidence.mkdir(exist_ok=True)
record = {'purpose': __doc__, 'seed': args.seed, 'scale': 'full', 'official': False,
          'entrant_runtime_checked': False, 'complete': False, 'tracks': {},
          'official_revision': subprocess.check_output(['git', '-C', str(OFFICIAL), 'rev-parse', 'HEAD'], text=True).strip()}


def sha(path):
    with path.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def save():
    record['updated_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    content = json.dumps(record, indent=2) + '\n'
    for target in [root / 'manifest.json', evidence / f'{args.seed}.json']:
        temporary = target.with_suffix('.tmp')
        temporary.write_text(content)
        temporary.replace(target)


try:
    for track, (module, output) in MODULE.items():
        data = SUB / f'.work/data/track-{track}/full-seed{args.seed}'
        reference = root / f'track-{track}'
        reference.mkdir(exist_ok=True)
        generator = OFFICIAL / f'generators/{module}.py'
        oracle = OFFICIAL / f'reference/{module}.py'
        item = {'generator_sha256': sha(generator), 'reference_sha256': sha(oracle), 'commands': []}
        record['tracks'][track] = item
        for argv in [[sys.executable, str(generator), '--seed', str(args.seed), '--scale', 'full', '--out', str(data)],
                     [sys.executable, str(oracle), '--data', str(data), '--out', str(reference)]]:
            p = subprocess.run(argv, capture_output=True, text=True, timeout=900)
            item['commands'].append({'argv': argv, 'exit': p.returncode, 'stdout': p.stdout[-2000:], 'stderr': p.stderr[-2000:]})
            assert p.returncode == 0, p.stderr[-2000:]
        item.update(inputs={p.name: sha(p) for p in data.iterdir() if p.is_file()},
                    data=str(data), reference_output=str(reference / output), output_name=output,
                    expected_sha256=sha(reference / output))
        save()
        print(track, 'ORACLE PREPARED', flush=True)
    record['complete'] = True
except Exception as error:
    record['failure'] = str(error)
save()
raise SystemExit(0 if record['complete'] else 1)
