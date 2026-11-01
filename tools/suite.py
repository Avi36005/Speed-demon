#!/usr/bin/env python3
"""Serial additional-seed differential checks; retain generated data outside tracks."""
import argparse
import datetime
import json
from pathlib import Path
import subprocess
import sys

SUB = Path(__file__).resolve().parents[1]
ROOT = SUB.parent
MODULE = {"a": "a_ingest", "b": "b_index", "c": "c_join"}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("track", choices=MODULE)
    ap.add_argument("--seeds", type=int, nargs="+", default=[2, 3, 42])
    ap.add_argument("--scale", choices=["tiny", "sample", "full"], default="sample")
    ap.add_argument("--fixtures", action="store_true")
    ap.add_argument("--workers", type=int)
    args = ap.parse_args()
    records = []
    commands = []
    if args.fixtures:
        sys.path.insert(0, str(SUB / "tests"))
        import fixtures
        labels = getattr(fixtures, "make_" + args.track)()
        for label in labels:
            command = [sys.executable, SUB / "tools/dev.py", args.track, "--data",
                       SUB / ".work/fixtures" / args.track / label, "--label", label]
            if label != "wide-sum":
                command += ["--sanitize"]
            commands.append(command)
    for seed in args.seeds:
        label = f"{args.scale}-seed{seed}"
        data = SUB / ".work/data" / f"track-{args.track}" / label
        generator = ROOT / "official/speed-demon-2026/generators" / (MODULE[args.track] + ".py")
        commands += [[sys.executable, generator, "--seed", str(seed), "--scale", args.scale, "--out", data],
                     [sys.executable, SUB / "tools/dev.py", args.track, "--data", data, "--label", label]]
    passed = True
    for command in commands:
        command = [str(x) for x in command]
        if args.workers is not None and str(SUB / "tools/dev.py") in command:
            command += ["--workers", str(args.workers)]
        proc = subprocess.run(command, capture_output=True, text=True)
        records.append({"argv": command, "exit": proc.returncode, "stdout": proc.stdout, "stderr": proc.stderr})
        label = command[command.index("--label") + 1] if "--label" in command else command[-1]
        print(label, "PASS" if proc.returncode == 0 else "FAIL", flush=True)
        if proc.returncode:
            print(proc.stdout, proc.stderr, flush=True)
            passed = False
            break
    evidence = SUB / "benchmarks" / f"track-{args.track}" / "suites.jsonl"
    evidence.parent.mkdir(parents=True, exist_ok=True)
    with evidence.open("a") as f:
        f.write(json.dumps({"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                            "passed": passed, "commands": records}) + "\n")
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
