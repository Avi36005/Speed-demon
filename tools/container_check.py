#!/usr/bin/env python3
"""Local amd64 emulation of sandbox checks. No official cgroup/board claims."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import uuid

SUB = Path(__file__).resolve().parents[1]
ROOT = SUB.parent
OFFICIAL = ROOT / "official/speed-demon-2026"
sys.path.insert(0, str(OFFICIAL / "harness"))
import dockerfile_check


def sha(path):
    with open(path, "rb") as f:
        return hashlib.file_digest(f, "sha256").hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("track", choices="acbd")
    ap.add_argument("--data", type=Path)
    ap.add_argument("--expected")
    ap.add_argument("--runs", type=int, default=5)
    ap.add_argument("--skip-build", action="store_true")
    ap.add_argument("--context", type=Path, help="build from an archived clean track folder")
    ap.add_argument("--tag", help="distinct local image tag for an isolated build experiment")
    ap.add_argument("--qemu", action="store_true", help="run the exact amd64 image through prepared native ARM QEMU")
    args = ap.parse_args()
    track = (args.context or SUB / f"track-{args.track}").resolve()
    data = (args.data or ROOT / f"data/track-{args.track}/full-seed1").resolve()
    manifest = json.loads((ROOT / "docs/SHA256SUMS.json").read_text())["tracks"][args.track]
    name = manifest["output"]
    expected = args.expected or manifest["expected_output_sha256"][0]
    tag = args.tag or f"kryptonite/track-{args.track}:local"
    record = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "official": False,
              "environment": "linux/amd64 emulated on ARM64 Docker Desktop",
              "track": args.track, "source_sha256": sha(track / "main.cpp"),
              "build_context": str(track),
              "image_tag": tag,
              "context_files_sha256": {str(p.relative_to(track)): sha(p)
                                       for p in sorted(track.rglob('*'))
                                       if p.is_file() and p.suffix.lower() != '.md'},
              "dockerfile_sha256": sha(track / "Dockerfile"), "data": str(data),
              "expected_sha256": expected, "commands": [], "runs": [],
              "official_cgroup_memory": None, "board_run_id": None}
    qemu = SUB / '.work/qemu-runtime'
    if args.qemu:
        record['environment'] = 'exact linux/amd64 image through native ARM QEMU Haswell; not native Intel timing'
        record['emulator'] = json.loads((qemu / 'metadata.json').read_text())
    work = SUB / ".work/containers" / (args.track + "-" + uuid.uuid4().hex[:10])
    work.mkdir(parents=True)
    record["artifact_dir"] = str(work.relative_to(SUB))

    def run(cmd, timeout=600):
        began = time.perf_counter()
        proc = subprocess.run([str(x) for x in cmd], capture_output=True, text=True, timeout=timeout)
        entry = {"argv": [str(x) for x in cmd], "exit": proc.returncode,
                 "outside_s": time.perf_counter() - began, "stdout": proc.stdout, "stderr": proc.stderr}
        record["commands"].append(entry)
        if proc.returncode:
            raise RuntimeError(proc.stderr[-3000:] or proc.stdout[-3000:])
        return entry

    try:
        allowlist = json.loads((OFFICIAL / "harness/allowlist.json").read_text())["images"]
        problems = dockerfile_check.check((track / "Dockerfile").read_text(), allowlist)
        assert not problems, problems
        record["inputs"] = {p.name: sha(p) for p in sorted(data.iterdir()) if p.is_file()}
        if not args.data:
            assert record["inputs"] == manifest["inputs_sha256"]
        if not args.skip_build:
            built = run(["docker", "build", "--platform", "linux/amd64", "--network=none", "--no-cache",
                         "-t", tag, track])
            record["build_s"] = built["outside_s"]
        details = json.loads(run(["docker", "image", "inspect", tag])["stdout"])[0]
        assert details["Architecture"] == "amd64"
        assert details["Size"] <= 2 * 1024**3
        assert not details["Config"].get("Volumes")
        assert details["Config"].get("Entrypoint") == ["/app/solution"]
        assert not details["Config"].get("Cmd")
        record["image_bytes"] = details["Size"]
        record["image_id"] = details["Id"]
        record["runtime_libraries"] = run(["docker", "run", "--rm", "--platform", "linux/amd64",
                                             "--network=none", "--entrypoint", "ldd", tag,
                                             "/app/solution"])["stdout"]
        timer = SUB / ".work/timer"
        timer.mkdir(exist_ok=True)
        timer_source = OFFICIAL / "harness/timer/sd-timer.c"
        record["timer_source_sha256"] = sha(timer_source)
        gcc = next(e for e in allowlist if e["image"] == "gcc:15")
        gcc_image = gcc["image"] + "@" + gcc["digest"]
        if not (timer / "sd-timer").exists():
            run(["docker", "run", "--rm", "--platform", "linux/amd64", "--network=none",
                 "-v", f"{timer_source}:/src/sd-timer.c:ro", "-v", f"{timer}:/build", gcc_image,
                 "gcc", "-O2", "-static", "/src/sd-timer.c", "-o", "/build/sd-timer"])
        for iteration in range(args.runs):
            out, sd = work / f"out{iteration}", work / f"sd{iteration}"
            out.mkdir(mode=0o777)
            os.chmod(out, 0o777)
            sd.mkdir(mode=0o700)
            container = "kryptonite-check-" + uuid.uuid4().hex[:12]
            cmd = ["docker", "run", "--name", container, "--platform", "linux/amd64",
                   "--no-healthcheck", "--network=none", "--read-only", "--tmpfs", "/tmp:rw,size=1g,exec",
                   "--memory=8g", "--memory-swap=8g", "--pids-limit=512", "--user=" + ("65534:65534" if args.qemu else "0:0"),
                   "--cap-drop=ALL", "--security-opt=no-new-privileges",
                   "--ulimit", "nofile=8192:8192", "-v", f"{data}:/data:ro", "-v", f"{out}:/out:rw",
                   "-v", f"{sd}:/sd", "-v", f"{timer}:/sd-bin:ro", "-e", f"TRACK={args.track}"]
            if args.qemu:
                # Compatibility mode executes as nobody directly. The nested
                # emulated timer rejected its argument contract in this setup;
                # leave the child interval unknown and retain Docker elapsed.
                cmd += ['-v', f'{qemu / "bin"}:/qemu:ro', '--entrypoint', '/qemu/qemu-x86_64',
                        tag, '-cpu', 'Haswell-noTSX', '-L', '/', '/app/solution']
            else:
                cmd += ['--cap-add=SETUID', '--cap-add=SETGID', '--entrypoint', '/sd-bin/sd-timer',
                        tag, '/sd/timing.json', '65534', '65534', '--', '/app/solution']
            try:
                done = run(cmd)
                diff = run(["docker", "diff", container])["stdout"].splitlines()
                diff = [line for line in diff if line.split(maxsplit=1)[-1] not in
                        ("/data", "/out", "/sd", "/sd-bin", "/tmp", "/qemu")]
                assert not diff, diff
                assert sorted(p.name for p in out.iterdir()) == [name]
                if args.qemu:
                    result = {"wall_ns": None, "exit_code": 0, "signal": 0,
                              "timing_method": "child interval unknown; outside_s includes Docker and QEMU startup"}
                else:
                    result = json.loads((sd / "timing.json").read_text())
                    assert result["exit_code"] == 0 and result["signal"] == 0
                result["output_sha256"] = sha(out / name)
                assert result["output_sha256"] == expected
                result["outside_s"] = done["outside_s"]
                result["fs_diff"] = diff
                record["runs"].append(result)
            finally:
                run(["docker", "rm", "-fv", container])
        record["passed"] = True
    except Exception as e:
        record["passed"] = False
        record["failure"] = str(e)
    (work / "record.json").write_text(json.dumps(record, indent=2) + "\n")
    log = SUB / "benchmarks" / f"track-{args.track}" / "containers.jsonl"
    log.parent.mkdir(parents=True, exist_ok=True)
    with log.open("a") as f:
        f.write(json.dumps(record) + "\n")
    print(json.dumps({k: v for k, v in record.items() if k != "commands"}, indent=2))
    return 0 if record["passed"] else 1


if __name__ == "__main__":
    sys.exit(main())
