#!/usr/bin/env python3
"""Source-matched full-seed7 correctness via prepared QEMU; no performance claims."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import uuid

SUB = Path(__file__).resolve().parents[1]
MODULE = dict(a='a_ingest', b='b_index', c='c_join', d='d_traverse')
OUTPUT = dict(a='summary.txt', b='results.txt', c='aggregates.txt', d='distances.txt')


def sha(path):
    with path.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--b-source', type=Path)
    ap.add_argument('--tracks', default='abcd')
    args = ap.parse_args()
    work = SUB / '.work/fresh-qemu' / uuid.uuid4().hex[:10]
    work.mkdir(parents=True)
    record = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'method': __doc__, 'official': False, 'checks': [], 'oracles': {}}
    try:
        assert args.tracks and all(t in MODULE for t in args.tracks)
        for track in args.tracks:
            data = SUB / f'.work/data/track-{track}/full-seed7'
            inputs = {p.name: sha(p) for p in data.iterdir() if p.is_file()}
            oracle = SUB.parent / f'official/speed-demon-2026/reference/{MODULE[track]}.py'
            expected = None
            for raw in reversed((SUB / f'benchmarks/track-{track}/experiments.jsonl').read_text().splitlines()):
                prior = json.loads(raw)
                if not prior.get('passed') or prior.get('inputs') != inputs:
                    continue
                path = SUB / prior['artifact_dir'] / 'reference' / OUTPUT[track]
                command = next((c for c in prior.get('commands', [])
                                if c.get('exit') == 0 and str(oracle) in c.get('argv', [])), None)
                if command and path.exists() and sha(path) == prior.get('expected_sha256'):
                    expected = sha(path)
                    record['oracles'][track] = {'method': 'reused verified untouched-reference bytes',
                                                'prior_record_utc': prior['utc'], 'command': command,
                                                'reference_output': str(path)}
                    break
            if expected is None:
                folder = work / f'reference-{track}'
                folder.mkdir()
                command = [sys.executable, str(oracle), '--data', str(data), '--out', str(folder)]
                p = subprocess.run(command, capture_output=True, text=True, check=True)
                expected = sha(folder / OUTPUT[track])
                record['oracles'][track] = {'method': 'fresh untouched reference', 'argv': command,
                                            'exit': p.returncode, 'stdout': p.stdout, 'stderr': p.stderr}
            record['oracles'][track].update(inputs=inputs, reference_source_sha256=sha(oracle),
                                            expected_sha256=expected)
            source = (args.b_source if track == 'b' and args.b_source else SUB / f'track-{track}/main.cpp').resolve()
            for workers in ([12, 1] if track == 'b' else [12]):
                command = [sys.executable, str(SUB / 'tools/cached_runtime.py'), track,
                           '--qemu', '--source', str(source), '--data', str(data), '--expected', expected,
                           '--workers', str(workers), '--runtime-timeout', '180']
                if track in 'abc':
                    command += ['--cxx-flag=-mavx2']
                if track in 'bc':
                    command += ['--cxx-flag=-mbmi2']
                p = subprocess.run(command, capture_output=True, text=True)
                record['checks'].append({'argv': command, 'exit': p.returncode,
                                          'source_sha256': sha(source), 'workers': workers,
                                          'stdout': p.stdout, 'stderr': p.stderr})
                print(track, workers, 'PASS' if p.returncode == 0 else 'FAIL', flush=True)
                if p.returncode:
                    raise RuntimeError(p.stdout[-3000:] + p.stderr[-1000:])
        record['passed'] = True
    except Exception as error:
        record['passed'] = False
        record['failure'] = str(error)
    (work / 'record.json').write_text(json.dumps(record, indent=2) + '\n')
    with (SUB / 'benchmarks/fresh-qemu.jsonl').open('a') as f:
        f.write(json.dumps(record) + '\n')
    print(json.dumps({k: v for k, v in record.items() if k != 'checks'}, indent=2))
    return 0 if record['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
