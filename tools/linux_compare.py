#!/usr/bin/env python3
"""Paired amd64-emulated screening; only the real board establishes Intel gains."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import uuid

SUB = Path(__file__).resolve().parents[1]
GCC = 'gcc:15@sha256:eb5e64286ad47187b32bda75c6b6916166ecb41da56559b8b09e0001748d2cce'
DEBIAN = 'debian:13-slim@sha256:62bd423aeee912639ca8ee681c2b762cb3e25137e0a92b20be455898a9f034ad'
OUTPUT = dict(a='summary.txt', b='results.txt', c='aggregates.txt', d='distances.txt')


def sha(path):
    with path.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('track', choices=OUTPUT)
    ap.add_argument('--candidate', type=Path, required=True)
    ap.add_argument('--baseline', type=Path)
    ap.add_argument('--hypothesis', required=True)
    ap.add_argument('--workers', type=int, default=12)
    ap.add_argument('--candidate-workers', type=int, help='explicit worker-count experiment; baseline keeps --workers')
    ap.add_argument('--runs', type=int, default=5)
    ap.add_argument('--qemu', action='store_true', help='use prepared native ARM QEMU compatibility backend')
    args = ap.parse_args()
    work = SUB / '.work/linux-compare' / (args.track + '-' + uuid.uuid4().hex[:10])
    work.mkdir(parents=True)
    data = SUB.parent / f'data/track-{args.track}/full-seed1'
    manifest = json.loads((SUB.parent / 'docs/SHA256SUMS.json').read_text())['tracks'][args.track]
    record = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'official': False, 'environment': 'linux/amd64 emulated on ARM64 Docker Desktop',
              'hypothesis': args.hypothesis, 'workers': args.workers, 'track': args.track,
              'artifact_dir': str(work.relative_to(SUB)), 'commands': [], 'versions': {},
              'memory_method': 'local container cgroup memory.peak including development wrapper; not official M',
              'time_method': 'unchanged official sd-timer child interval; emulated, not native Intel T',
              'decision': 'pending review', 'board_run_id': None}

    def run(cmd, timeout=180):
        cmd = [str(x) for x in cmd]
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        record['commands'].append({'argv': cmd, 'exit': p.returncode,
                                   'stdout': p.stdout, 'stderr': p.stderr})
        if p.returncode:
            raise RuntimeError(p.stderr[-4000:] or p.stdout[-4000:])
        return p

    try:
        record['inputs'] = {p.name: sha(p) for p in data.iterdir() if p.is_file()}
        assert record['inputs'] == manifest['inputs_sha256']
        record['expected_sha256'] = expected = manifest['expected_output_sha256'][0]
        record['git_context_revision'] = run(['git', 'rev-parse', 'HEAD']).stdout.strip()
        record['compiler'] = run(['docker', 'run', '--rm', '--platform', 'linux/amd64',
                                  '--network=none', GCC, 'g++', '--version']).stdout
        flags = ['-std=c++20', '-O3', '-DNDEBUG', '-pthread', '-static-libstdc++', '-static-libgcc']
        if args.track in 'abc':
            flags += ['-mavx2']
        if args.track in 'bc':
            flags += ['-mbmi2']
        record['flags'] = flags
        qemu = SUB / '.work/qemu-runtime'
        if args.qemu:
            metadata = json.loads((qemu / 'metadata.json').read_text())
            record['environment'] = 'native ARM64 QEMU TCG emulating x86 Haswell; screening only'
            record['qemu'] = metadata
            record['time_method'] = 'native ARM date CLOCK_REALTIME interval around QEMU; includes translator startup, not Intel T'
            record['memory_method'] = 'local cgroup peak including QEMU and wrapper, not official M'
        for name, source in [('baseline', args.baseline or SUB / f'track-{args.track}/main.cpp'),
                             ('candidate', args.candidate)]:
            folder = work / name
            folder.mkdir()
            (folder / 'main.cpp').write_bytes(source.read_bytes())
            record['versions'][name] = {'source': str(source.resolve()),
                                        'source_sha256': sha(folder / 'main.cpp'), 'samples': [],
                                        'workers': (args.candidate_workers if name == 'candidate' and args.candidate_workers is not None else args.workers)}
            run(['docker', 'run', '--rm', '--platform', 'linux/amd64', '--network=none',
                 '-v', f'{folder}:/build', GCC, 'g++', *flags, '/build/main.cpp', '-o', '/build/solution'])
        timer = SUB / '.work/timer'
        if not args.qemu:
            assert (timer / 'sd-timer').exists(), 'prepare unchanged official timer first'
            record['timer_binary_sha256'] = sha(timer / 'sd-timer')
        # The parent shell reads output/peak AFTER the timer has reaped the child.
        # /out is tmpfs, as on the board; no output file survives container exit.
        wrapper = ('/timer/sd-timer /sd/timing.json 65534 65534 -- /candidate/solution --workers "$WORKERS"; '
                   'result=$?; sha256sum /out/"$OUTPUT"; '
                   'printf "MEMORY_PEAK "; cat /sys/fs/cgroup/memory.peak 2>/dev/null || echo unknown; '
                   'exit "$result"')
        if args.qemu:
            wrapper = ('start=$(date +%s%N); '
                       '/qemu/bin/qemu-x86_64 -cpu Haswell-noTSX -L /qemu/amd64-root '
                       '/candidate/solution --workers "$WORKERS"; result=$?; end=$(date +%s%N); '
                       'printf "WALL_NS %s\n" "$((end-start))"; sha256sum /out/"$OUTPUT"; '
                       'printf "MEMORY_PEAK "; cat /sys/fs/cgroup/memory.peak 2>/dev/null || echo unknown; '
                       'exit "$result"')
        for iteration in range(args.runs + 1):
            order = ['baseline', 'candidate'] if iteration % 2 == 0 else ['candidate', 'baseline']
            for name in order:
                sd = work / f'{name}-{iteration}'
                sd.mkdir()
                command = ['docker', 'run', '--rm', '--platform', 'linux/arm64' if args.qemu else 'linux/amd64', '--network=none',
                         '--read-only', '--tmpfs', '/tmp:rw,size=1g,exec',
                         '--tmpfs', '/out:rw,size=1g,mode=1777', '--memory=8g', '--memory-swap=8g',
                         '--pids-limit=512', '--cap-drop=ALL', '--cap-add=SETUID', '--cap-add=SETGID',
                         '--security-opt=no-new-privileges', '-v', f'{work / name}:/candidate:ro',
                         '-v', f'{data}:/data:ro', '-v', f'{sd}:/sd', '-v', f'{timer}:/timer:ro',
                         '-e', f'WORKERS={record["versions"][name]["workers"]}', '-e', f'OUTPUT={OUTPUT[args.track]}']
                if args.qemu:
                    command += ['--user=65534:65534', '-v', f'{qemu}:/qemu:ro', metadata['arm_runtime_image']]
                else:
                    command += [DEBIAN]
                p = run(command + ['sh', '-c', wrapper])
                if args.qemu:
                    elapsed = next(line.split()[-1] for line in p.stdout.splitlines() if line.startswith('WALL_NS '))
                    timing = {'wall_ns': int(elapsed), 'exit_code': 0, 'signal': 0}
                else:
                    timing = json.loads((sd / 'timing.json').read_text())
                assert timing['exit_code'] == 0 and timing['signal'] == 0
                assert any(line.split()[0] == expected for line in p.stdout.splitlines() if line.split())
                memory = next((line.split()[-1] for line in p.stdout.splitlines()
                               if line.startswith('MEMORY_PEAK ')), 'unknown')
                sample = {'wall_ns': timing['wall_ns'], 'output_sha256': expected,
                          'cgroup_peak_bytes': int(memory) if memory.isdigit() else None,
                          'workers': record['versions'][name]['workers'],
                          'warmup': iteration == 0}
                record['versions'][name]['samples'].append(sample)
                print(name, iteration, timing['wall_ns'], sample['cgroup_peak_bytes'], flush=True)
        for version in record['versions'].values():
            times = sorted(s['wall_ns'] / 1e9 for s in version['samples'] if not s['warmup'])
            selected = times[1:-1] if len(times) >= 5 else times
            version['middle_mean_s'] = sum(selected) / len(selected)
            memory = [s['cgroup_peak_bytes'] for s in version['samples']
                      if not s['warmup'] and s['cgroup_peak_bytes'] is not None]
            version['peak_cgroup_bytes'] = max(memory) if memory else None
        record['passed'] = True
    except Exception as error:
        record['passed'] = False
        record['failure'] = str(error)
    (work / 'record.json').write_text(json.dumps(record, indent=2) + '\n')
    with (SUB / f'benchmarks/track-{args.track}/linux-comparisons.jsonl').open('a') as f:
        f.write(json.dumps(record) + '\n')
    print(json.dumps({k: v for k, v in record.items() if k != 'commands'}, indent=2))
    return 0 if record['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
