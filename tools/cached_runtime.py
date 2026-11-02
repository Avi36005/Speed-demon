#!/usr/bin/env python3
"""Linux runtime proof from cached pinned bases; explicitly NOT a clean Dockerfile build."""
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
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('track', choices=OUTPUT)
    ap.add_argument('--source', type=Path)
    ap.add_argument('--fixtures', action='store_true')
    ap.add_argument('--data', type=Path)
    ap.add_argument('--expected')
    group = ap.add_mutually_exclusive_group()
    group.add_argument('--sanitize', action='store_true')
    group.add_argument('--ubsan', action='store_true', help='UBSan without ASan shadow-address reservations')
    ap.add_argument('--workers', type=int)
    ap.add_argument('--runtime-timeout', type=int, default=30)
    ap.add_argument('--cxx-flag', action='append', default=[], help='additional explicit compiler flag, recorded verbatim')
    ap.add_argument('--qemu', action='store_true', help='use prepared native ARM QEMU Haswell model instead of the default translator')
    args = ap.parse_args()
    source = (args.source or SUB / f'track-{args.track}/main.cpp').resolve()
    work = SUB / '.work/cached-runtime' / (args.track + '-' + uuid.uuid4().hex[:10])
    build = work / 'build'
    build.mkdir(parents=True)
    (build / 'main.cpp').write_bytes(source.read_bytes())
    record = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'method': __doc__, 'environment': 'linux/amd64 emulated on ARM64 Docker Desktop',
              'official': False, 'track': args.track, 'workers': args.workers, 'source_sha256': sha(source),
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
        qemu = SUB / '.work/qemu-runtime'
        if args.qemu:
            metadata = json.loads((qemu / 'metadata.json').read_text())
            record['environment'] = 'native ARM64 QEMU TCG emulating x86 Haswell; compatibility only'
            record['qemu'] = metadata
            record['runtime_image'] = metadata['arm_runtime_image']
            record['x86_libraries_image'] = DEBIAN
        flags = ['-std=c++20', '-pthread', '-static-libstdc++', '-static-libgcc']
        if args.sanitize or args.ubsan:
            flags += ['-O1', '-g', '-fsanitize=' + ('undefined' if args.ubsan else 'address,undefined'),
                      '-static-libubsan', '-fno-omit-frame-pointer']
            if args.sanitize:
                flags += ['-static-libasan']
        else:
            flags += ['-O3', '-DNDEBUG']
        flags += args.cxx_flag
        record['flags'] = flags
        # Static UBSan may introduce C++ ABI references after libstdc++ has
        # already been scanned. Rescan these static archives as one group.
        link_flags = ['-Wl,--start-group', '-Wl,-Bstatic', '-lstdc++', '-lubsan',
                      '-Wl,-Bdynamic', '-Wl,--end-group'] if args.sanitize or args.ubsan else []
        record['link_flags'] = link_flags
        run(['docker', 'run', '--rm', '--pull=never', '--platform', 'linux/amd64',
             '--network=none', '-v', f'{build}:/build', GCC,
             'g++', *flags, '/build/main.cpp', *link_flags, '-o', '/build/solution'])
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
                data = SUB / '.work/fixtures' / args.track / label
                hand = SUB / '.work/fixture-answers' / args.track / label
                if hand.exists():
                    expected = sha(hand)
                    basis = {'method': 'worked or independently derived fixture bytes', 'path': str(hand)}
                else:
                    module = dict(a='a_ingest', b='b_index', c='c_join', d='d_traverse')[args.track]
                    oracle = SUB.parent / 'official/speed-demon-2026/reference' / (module + '.py')
                    reference = work / ('reference-' + label)
                    reference.mkdir()
                    run([sys.executable, oracle, '--data', data, '--out', reference])
                    expected = sha(reference / OUTPUT[args.track])
                    basis = {'method': 'fresh untouched official reference', 'source_sha256': sha(oracle),
                             'output': str(reference / OUTPUT[args.track])}
                record.setdefault('expected_provenance', {})[label] = basis
                cases.append((label, data, expected))
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
                cmd = ['docker', 'run', '--name', container, '--pull=never', '--platform', 'linux/arm64' if args.qemu else 'linux/amd64',
                       '--network=none', '--read-only', '--tmpfs', '/tmp:rw,size=1g,exec',
                       '--memory=8g', '--memory-swap=8g', '--pids-limit=512', '--user=65534:65534',
                       '--cap-drop=ALL', '--security-opt=no-new-privileges',
                       '-e', 'ASAN_OPTIONS=detect_leaks=0', '-e', 'UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1', '-v', f'{build}:/candidate:ro',
                       '-v', f'{data}:/data:ro', '-v', f'{out}:/out:rw']
                if args.qemu:
                    cmd += ['-v', f'{qemu}:/qemu:ro', '--entrypoint', '/qemu/bin/qemu-x86_64',
                            metadata['arm_runtime_image'], '-cpu', 'Haswell-noTSX', '-L',
                            '/qemu/amd64-root', '/candidate/solution']
                else:
                    cmd += [DEBIAN, '/candidate/solution']
                if args.workers is not None:
                    cmd += ['--workers', str(args.workers)]
                run(cmd, timeout=args.runtime_timeout)
                diff = run(['docker', 'diff', container]).splitlines()
                diff = [line for line in diff if line.split(maxsplit=1)[-1] not in
                        ('/candidate', '/data', '/out', '/tmp', '/qemu')]
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
        if isinstance(error, subprocess.TimeoutExpired):
            record['timeout_stdout'] = error.stdout.decode(errors='replace') if isinstance(error.stdout, bytes) else error.stdout
            record['timeout_stderr'] = error.stderr.decode(errors='replace') if isinstance(error.stderr, bytes) else error.stderr
        record['passed'] = False
        record['failure'] = str(error)
    (work / 'record.json').write_text(json.dumps(record, indent=2) + '\n')
    with (SUB / f'benchmarks/track-{args.track}/cached-runtime.jsonl').open('a') as stream:
        stream.write(json.dumps(record) + '\n')
    print(json.dumps({k: v for k, v in record.items() if k not in ('commands', 'checks')}, indent=2))
    return 0 if record['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
