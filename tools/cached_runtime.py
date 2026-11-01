#!/usr/bin/env python3
"""Linux runtime proof from cached pinned bases; explicitly NOT a clean Dockerfile build."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import subprocess
import uuid

SUB = Path(__file__).resolve().parents[1]
GCC = 'gcc:15@sha256:eb5e64286ad47187b32bda75c6b6916166ecb41da56559b8b09e0001748d2cce'
DEBIAN = 'debian:13-slim@sha256:62bd423aeee912639ca8ee681c2b762cb3e25137e0a92b20be455898a9f034ad'
OUTPUT = dict(a='summary.txt', b='results.txt', c='aggregates.txt', d='distances.txt')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('track', choices=OUTPUT)
    ap.add_argument('--source', type=Path)
    ap.add_argument('--fixtures', action='store_true')
    ap.add_argument('--data', type=Path)
    ap.add_argument('--expected')
    ap.add_argument('--sanitize', action='store_true')
    ap.add_argument('--workers', type=int)
    ap.add_argument('--runtime-timeout', type=int, default=30)
    args = ap.parse_args()
    source = (args.source or SUB / f'track-{args.track}/main.cpp').resolve()
    work = SUB / '.work/cached-runtime' / (args.track + '-' + uuid.uuid4().hex[:10])
    build = work / 'build'
    build.mkdir(parents=True)
    (build / 'main.cpp').write_bytes(source.read_bytes())
    record = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'method': __doc__, 'environment': 'linux/amd64 emulated on ARM64 Docker Desktop',
              'official': False, 'track': args.track, 'source_sha256': sha(source),
              'source': str(source), 'compiler_image': GCC, 'runtime_image': DEBIAN,
              'artifact_dir': str(work.relative_to(SUB)), 'clean_dockerfile_build': False,
              'commands': [], 'checks': []}

    def run(cmd, timeout=600):
        proc = subprocess.run([str(x) for x in cmd], capture_output=True, text=True, timeout=timeout)
        record['commands'].append({'argv': [str(x) for x in cmd], 'exit': proc.returncode,
                                   'stdout': proc.stdout[-4000:], 'stderr': proc.stderr[-4000:]})
        if proc.returncode:
            raise RuntimeError(proc.stderr[-3000:] or proc.stdout[-3000:])
        return proc.stdout

    try:
        flags = ['-std=c++20', '-pthread', '-static-libstdc++', '-static-libgcc']
        flags += ['-O1', '-g', '-fsanitize=address,undefined', '-static-libasan',
                  '-static-libubsan', '-fno-omit-frame-pointer'] if args.sanitize else ['-O3', '-DNDEBUG']
        record['flags'] = flags
        run(['docker', 'run', '--rm', '--pull=never', '--platform', 'linux/amd64',
             '--network=none', '-v', f'{build}:/build', GCC,
             'g++', *flags, '/build/main.cpp', '-o', '/build/solution'])
        record['binary_sha256'] = sha(build / 'solution')
        record['runtime_libraries'] = run([
            'docker', 'run', '--rm', '--pull=never', '--platform', 'linux/amd64',
            '--network=none', '-v', f'{build}:/candidate:ro', DEBIAN, 'ldd', '/candidate/solution'])
        cases = []
        if args.fixtures:
            import sys
            sys.path.insert(0, str(SUB / 'tests'))
            import fixtures
            for label in getattr(fixtures, 'make_' + args.track)():
                expected = sha(SUB / '.work/fixture-answers' / args.track / label)
                cases.append((label, SUB / '.work/fixtures' / args.track / label, expected))
        if args.data:
            assert args.expected, '--data requires --expected'
            cases.append((args.data.name, args.data.resolve(), args.expected))
        assert cases, 'provide --fixtures and/or --data --expected'
        for label, data, expected in cases:
            out = work / label
            out.mkdir(mode=0o777)
            os.chmod(out, 0o777)
            container = 'kryptonite-cached-' + uuid.uuid4().hex[:12]
            try:
                cmd = ['docker', 'run', '--name', container, '--pull=never', '--platform', 'linux/amd64',
                       '--network=none', '--read-only', '--tmpfs', '/tmp:rw,size=1g,exec',
                       '--memory=8g', '--memory-swap=8g', '--pids-limit=512', '--user=65534:65534',
                       '--cap-drop=ALL', '--security-opt=no-new-privileges',
                       '-e', 'ASAN_OPTIONS=detect_leaks=0', '-v', f'{build}:/candidate:ro',
                       '-v', f'{data}:/data:ro', '-v', f'{out}:/out:rw',
                       DEBIAN, '/candidate/solution']
                if args.workers is not None:
                    cmd += ['--workers', str(args.workers)]
                run(cmd, timeout=args.runtime_timeout)
                diff = run(['docker', 'diff', container]).splitlines()
                diff = [line for line in diff if line.split(maxsplit=1)[-1] not in
                        ('/candidate', '/data', '/out', '/tmp')]
                assert not diff, diff
                assert sorted(p.name for p in out.iterdir()) == [OUTPUT[args.track]]
                got = sha(out / OUTPUT[args.track])
                assert got == expected, (label, got, expected)
                record['checks'].append({'dataset': label, 'inputs': {p.name: sha(p) for p in data.iterdir()},
                                         'output_sha256': got, 'expected_sha256': expected,
                                         'sandbox_filesystem_diff': diff, 'passed': True})
                print(label, 'PASS', flush=True)
            finally:
                run(['docker', 'rm', '-fv', container])
        record['passed'] = True
    except Exception as error:
        record['passed'] = False
        record['failure'] = str(error)
    (work / 'record.json').write_text(json.dumps(record, indent=2) + '\n')
    with (SUB / f'benchmarks/track-{args.track}/cached-runtime.jsonl').open('a') as stream:
        stream.write(json.dumps(record) + '\n')
    print(json.dumps({k: v for k, v in record.items() if k not in ('commands', 'checks')}, indent=2))
    return 0 if record['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
