#!/usr/bin/env python3
"""Build an explicitly instrumented development copy; never use as scored timing."""
import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import uuid

SUB = Path(__file__).resolve().parents[1]
track = sys.argv[1]
source = SUB / f"track-{track}/main.cpp"
original = source.read_text()
text = '#include <chrono>\n' + original
begin = 'auto phase_start = std::chrono::steady_clock::now();\n'


def replace(old, new):
    global text
    assert text.count(old) == 1, old
    text = text.replace(old, new)


def phase(label):
    return ('std::cerr << "' + label + ' " << std::chrono::duration<double>('
            'std::chrono::steady_clock::now()-phase_start).count() << "\\n";\n'
            'phase_start=std::chrono::steady_clock::now();\n')


if track == 'a':
    replace('Input input(data_dir', begin + 'Input input(data_dir')
    replace('std::vector<std::string_view> order;', phase('ingest') + 'std::vector<std::string_view> order;')
    replace('std::ofstream out(', phase('sort_and_serialize') + 'std::ofstream out(')
elif track == 'c':
    replace('std::vector<Country> countries;', begin + 'std::vector<Country> countries;')
    replace('Input records(', phase('lookup') + 'Input records(')
    replace('std::sort(countries.begin()', phase('parse_and_aggregate') + 'std::sort(countries.begin()')
elif track == 'b':
    replace('Input query_file(', begin + 'Input query_file(')
    replace('std::vector<Posting> index(', phase('queries') + 'std::vector<Posting> index(')
    if 'std::vector<std::string> answers(' in text:
        replace('std::vector<std::string> answers(', phase('index') + 'std::vector<std::string> answers(')
        replace('std::ofstream out(', phase('evaluate_and_format') + 'std::ofstream out(')
        replace('if (!out) throw', phase('write') + 'if (!out) throw')
    else:
        replace('std::ofstream out(', phase('index') + 'std::ofstream out(')
        replace('if (!out) throw', phase('evaluate_and_serialize') + 'if (!out) throw')
else:
    raise ValueError(track)

work = SUB / '.work/profile' / (track + '-' + uuid.uuid4().hex[:10])
work.mkdir(parents=True)
(work / 'main.cpp').write_text(text)
(work / 'out').mkdir()
commands = []
for cmd in (["clang++", "-std=c++20", "-O3", "-DNDEBUG", "-pthread", work / 'main.cpp', '-o', work / 'solution'],
            [work / 'solution', '--data', SUB.parent / f'data/track-{track}/full-seed1', '--out', work / 'out', *sys.argv[2:]]):
    proc = subprocess.run([str(x) for x in cmd], capture_output=True, text=True)
    commands.append({'argv': [str(x) for x in cmd], 'exit': proc.returncode,
                     'stdout': proc.stdout, 'stderr': proc.stderr})
    if proc.returncode:
        break
record = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'diagnostic': True,
          'source_sha256': hashlib.sha256(original.encode()).hexdigest(),
          'instrumented_sha256': hashlib.sha256(text.encode()).hexdigest(),
          'artifact_dir': str(work.relative_to(SUB)), 'commands': commands,
          'outputs': {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in (work / 'out').iterdir()}}
with (SUB / f'benchmarks/track-{track}/profiles.jsonl').open('a') as f:
    f.write(json.dumps(record) + '\n')
print(json.dumps(record, indent=2))
sys.exit(commands[-1]['exit'])
