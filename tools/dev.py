#!/usr/bin/env python3
"""Correctness runner; separate optional process benchmarks. Python 3.11+."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import subprocess
import sys
import time

SUB = Path(__file__).resolve().parents[1]
ROOT = SUB.parent
OFFICIAL = ROOT / "official/speed-demon-2026"
NAMES = {"a": ("a_ingest", "summary.txt"), "b": ("b_index", "results.txt"),
         "c": ("c_join", "aggregates.txt")}


def digest(path):
    with open(path, "rb") as f:
        return hashlib.file_digest(f, "sha256").hexdigest()


def run(cmd, log):
    proc = subprocess.run([str(x) for x in cmd], capture_output=True)
    log.append({"argv": [str(x) for x in cmd], "exit": proc.returncode,
                "stdout": proc.stdout.decode(errors="replace"),
                "stderr": proc.stderr.decode(errors="replace")})
    if proc.returncode:
        raise RuntimeError(f"command failed: {cmd}: {proc.stderr.decode(errors='replace')}")
    return proc


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("track", choices=NAMES)
    ap.add_argument("--data", type=Path, required=True)
    ap.add_argument("--label", required=True)
    ap.add_argument("--repeat", type=int, default=0)
    ap.add_argument("--sanitize", action="store_true")
    ap.add_argument("--workers", type=int)
    ap.add_argument("--compiler", default="clang++")
    ap.add_argument("--binary", type=Path, help="use an already built binary (e.g. baseline)")
    args = ap.parse_args()
    module, name = NAMES[args.track]
    track = SUB / f"track-{args.track}"
    stamp = datetime.datetime.now(datetime.timezone.utc).isoformat()
    safe = re.sub(r"[^a-zA-Z0-9_.-]", "_", stamp + "-" + args.label)
    work = SUB / ".work" / f"track-{args.track}" / safe
    work.mkdir(parents=True)
    commands = []
    record = {"utc": stamp, "track": args.track, "dataset": args.label,
              "data": str(args.data.resolve()), "architecture": platform.machine(),
              "platform": platform.platform(), "source_sha256": digest(track / "main.cpp"),
              "workers": args.workers, "board_run_id": None, "official": False,
              "artifact_dir": str(work.relative_to(SUB)), "commands": commands,
              "time_samples_s": [], "memory_method": None, "peak_rss_bytes": None}
    try:
        record["inputs"] = {p.name: digest(p) for p in sorted(args.data.iterdir()) if p.is_file()}
        flags = ["-std=c++20", "-pthread", "-Wall", "-Wextra", "-Wpedantic"]
        flags += (["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
                  if args.sanitize else ["-O3", "-DNDEBUG"])
        binary = args.binary.resolve() if args.binary else work / "solution"
        record["compiler"] = run([args.compiler, "--version"], commands).stdout.decode().strip()
        record["flags"] = flags if not args.binary else None
        if not args.binary:
            run([args.compiler, *flags, track / "main.cpp", "-o", binary], commands)
        record["binary_sha256"] = digest(binary)
        candidate, reference = work / "candidate", work / "reference"
        candidate.mkdir()
        reference.mkdir()
        data = args.data.resolve()
        command = [binary, "--data", data, "--out", candidate]
        if args.workers is not None:
            command += ["--workers", str(args.workers)]
        run(command, commands)
        run([sys.executable, OFFICIAL / "reference" / f"{module}.py",
             "--data", data, "--out", reference], commands)
        if sorted(p.name for p in candidate.iterdir()) != [name]:
            raise RuntimeError("candidate must emit exactly " + name)
        got, expected = (candidate / name).read_bytes(), (reference / name).read_bytes()
        record["output_sha256"] = digest(candidate / name)
        record["expected_sha256"] = digest(reference / name)
        if got != expected:
            offset = next((i for i, (a, b) in enumerate(zip(got, expected)) if a != b),
                          min(len(got), len(expected)))
            diff = {"offset": offset, "line": got[:offset].count(b"\n") + 1,
                    "got_bytes": len(got), "expected_bytes": len(expected),
                    "got_context": repr(got[max(0, offset-80):offset+160]),
                    "expected_context": repr(expected[max(0, offset-80):offset+160])}
            (work / "first-difference.json").write_text(json.dumps(diff, indent=2))
            raise RuntimeError("byte mismatch: " + str(diff))
        if args.label == "full-seed1":
            published = json.loads((ROOT / "docs/SHA256SUMS.json").read_text())["tracks"][args.track]
            assert record["inputs"] == published["inputs_sha256"]
            assert record["output_sha256"] in published["expected_output_sha256"]
        record["correctness_passed"] = True
        # wait4 gives this child's RSS, without time(1)'s restricted macOS sysctl.
        # These local spawn-to-exit samples include launch overhead, never board T.
        for _ in range(args.repeat):
            with open(work / "timed.stdout", "wb") as stdout, open(work / "timed.stderr", "wb") as stderr:
                began = time.perf_counter()
                proc = subprocess.Popen([str(x) for x in command], stdout=stdout, stderr=stderr)
                _, status, usage = os.wait4(proc.pid, 0)
                elapsed = time.perf_counter() - began
                proc.returncode = os.waitstatus_to_exitcode(status)
            commands.append({"argv": [str(x) for x in command], "exit": proc.returncode,
                             "phase": "benchmark", "stdout_file": "timed.stdout",
                             "stderr_file": "timed.stderr"})
            if proc.returncode:
                raise RuntimeError("timed candidate failed: " + (work / "timed.stderr").read_text())
            record["time_samples_s"].append(elapsed)
            rss = usage.ru_maxrss * (1 if sys.platform == "darwin" else 1024)
            record["peak_rss_bytes"] = max(record["peak_rss_bytes"] or 0, rss)
            record["memory_method"] = "wait4 ru_maxrss; local process RSS, not cgroup memory"
            if digest(candidate / name) != record["expected_sha256"]:
                raise RuntimeError("nondeterministic output in repeated run")
        record["passed"] = True
    except Exception as e:
        record["passed"] = False
        record["failure"] = str(e)
    finally:
        evidence = SUB / "benchmarks" / f"track-{args.track}"
        evidence.mkdir(parents=True, exist_ok=True)
        (work / "record.json").write_text(json.dumps(record, indent=2) + "\n")
        with open(evidence / "experiments.jsonl", "a") as f:
            f.write(json.dumps(record) + "\n")
    print(json.dumps({k: v for k, v in record.items() if k not in ("commands", "inputs")}, indent=2))
    return 0 if record["passed"] else 1


if __name__ == "__main__":
    sys.exit(main())
