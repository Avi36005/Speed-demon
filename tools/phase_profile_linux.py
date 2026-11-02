#!/usr/bin/env python3
"""Coarse source-matched phase diagnostics through QEMU; not Intel timing."""
import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import uuid

SUB = Path(__file__).resolve().parents[1]
track = sys.argv[1]
assert track in 'bc'
source = SUB / f'track-{track}/main.cpp'
original = source.read_text()
text = '#include <chrono>\n' + original


def replace(old, new):
    global text
    assert text.count(old) == 1, old
    text = text.replace(old, new)


def lap(label):
    return ('fprintf(stderr,"PHASE ' + label + ' %.9f\\n",'
            'std::chrono::duration<double>(Clock::now()-mark).count()); mark=Clock::now();\n')


replace('    // The scoring box', '    using Clock = std::chrono::steady_clock;\n'
        '    auto mark=Clock::now();\n    // The scoring box')
if track == 'b':
    replace('        const size_t V = vocab.keys.size();', lap('map_and_query_vocabulary') +
            '        const size_t V = vocab.keys.size();')
    replace('        // Phrases are matched during the scan,', lap('cuts_sample_and_dense_choice') +
            '        // Phrases are matched during the scan,')
    replace('        for (size_t ph = 0; ph < phases; ++ph) {\n        pbase',
            lap('phrase_plan') + 'double scan_s=0, metadata_s=0, evaluate_s=0;\n'
            '        for (size_t ph = 0; ph < phases; ++ph) {\n        pbase')
    replace('        // Document frequencies:',
            'scan_s+=std::chrono::duration<double>(Clock::now()-mark).count(); mark=Clock::now();\n'
            '        // Document frequencies:')
    replace('        threads.run(workers, [&](size_t) {',
            'metadata_s+=std::chrono::duration<double>(Clock::now()-mark).count(); mark=Clock::now();\n'
            '        threads.run(workers, [&](size_t) {')
    replace('        }  // phases',
            'evaluate_s+=std::chrono::duration<double>(Clock::now()-mark).count(); mark=Clock::now();\n'
            '        }  // phases\n'
            'fprintf(stderr,"PHASE scan_and_inline_phrases %.9f\\nPHASE block_metadata %.9f\\nPHASE bool_evaluation %.9f\\n", scan_s,metadata_s,evaluate_s);')
    replace('        int fd = open((out_dir', lap('phrase_merge_and_serialize') + '        int fd = open((out_dir')
elif track == 'c':
    replace('        // Exact chunk starts:', lap('map_and_regions') + '        // Exact chunk starts:')
    replace('        run([&](size_t w) {\n            auto& g', lap('cuts_and_allocations') +
            '        run([&](size_t w) {\n            auto& g')
    replace('        run([&](size_t w) { drop(', lap('parse_intern_aggregate') + '        run([&](size_t w) { drop(')
    replace('        std::vector<uint64_t> distinct(G);', lap('parallel_input_drop') +
            '        std::vector<uint64_t> distinct(G);')
    replace('        FILE* f = fopen(', lap('merge_popcount_serialize') + '        FILE* f = fopen(')

work = SUB / '.work/phase-profile' / (track + '-' + uuid.uuid4().hex[:10])
work.mkdir(parents=True)
candidate = work / 'main.cpp'
candidate.write_text(text)
expected = json.loads((SUB.parent / 'docs/SHA256SUMS.json').read_text())['tracks'][track]['expected_output_sha256'][0]
cmd = [sys.executable, str(SUB / 'tools/cached_runtime.py'), track, '--qemu', '--source', str(candidate),
       '--data', str(SUB.parent / f'data/track-{track}/full-seed1'), '--expected', expected,
       '--workers', sys.argv[2] if len(sys.argv) > 2 else '12', '--runtime-timeout', '180',
       '--cxx-flag=-mavx2', '--cxx-flag=-mbmi2']
result = subprocess.run(cmd, capture_output=True, text=True)
record = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'diagnostic': True,
          'official': False, 'method': __doc__, 'source_sha256': hashlib.sha256(original.encode()).hexdigest(),
          'instrumented_sha256': hashlib.sha256(text.encode()).hexdigest(), 'artifact_dir': str(work.relative_to(SUB)),
          'argv': cmd, 'exit': result.returncode, 'stdout': result.stdout, 'stderr': result.stderr}
matches = [json.loads(line) for line in (SUB / f'benchmarks/track-{track}/cached-runtime.jsonl').read_text().splitlines()
           if json.loads(line)['source_sha256'] == record['instrumented_sha256']]
if matches:
    proof = matches[-1]
    record['cached_runtime_proof'] = proof['artifact_dir']
    record['correctness_passed'] = proof.get('passed', False)
    record['phase_stderr'] = [c['stderr'] for c in proof['commands'] if 'PHASE ' in c['stderr']]
(work / 'record.json').write_text(json.dumps(record, indent=2) + '\n')
with (SUB / f'benchmarks/track-{track}/profiles.jsonl').open('a') as f:
    f.write(json.dumps(record) + '\n')
print(json.dumps(record, indent=2))
raise SystemExit(result.returncode)
