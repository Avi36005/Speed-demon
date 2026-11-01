#!/usr/bin/env python3
"""Save this team's public live-board evidence without treating it as final judging."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import urllib.request

SUB = Path(__file__).resolve().parents[1]
SOURCE = "https://s3.amazonaws.com/raptors.dev/speeddemon.dev/2026/board/board.json"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--team", default="Kryptonite")
    args = parser.parse_args()
    fetch_url = SOURCE + '?checked=' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d%H%M%S')
    request = urllib.request.Request(fetch_url, headers={"Cache-Control": "no-cache", "Accept-Encoding": "identity"})
    with urllib.request.urlopen(request, timeout=30) as response:
        raw = response.read()
        length = response.headers.get('Content-Length')
        if length is not None and len(raw) != int(length):
            raise RuntimeError(f'incomplete feed: received {len(raw)} of {length} bytes; previous receipt preserved')
        feed = json.loads(raw)
    teams = [team for team in feed["teams"] if team["team"] == args.team]
    now = datetime.datetime.now(datetime.timezone.utc)
    receipt = {"checked_at_utc": now.isoformat(), "source_url": SOURCE, "retrieved_url": fetch_url,
               "feed_generated_at": feed.get("generated_at"), "teams": teams,
               "measurement_scope": "single live public-data run; final holdout/judging separate",
               "runtime_inputs_by_commit": {}}
    for team in teams:
        for track, entry in team["tracks"].items():
            for kind in ("best", "latest"):
                run = entry.get(kind)
                if not run or not run.get("commit"):
                    continue
                commit = run["commit"]
                digests = receipt["runtime_inputs_by_commit"].setdefault(commit, {})
                for name in ("main.cpp", "Dockerfile", ".speed-demon.toml"):
                    path = f"track-{track}/{name}"
                    proc = subprocess.run(["git", "show", f"{commit}:{path}"], cwd=SUB,
                                          capture_output=True)
                    if proc.returncode == 0:
                        digests[path] = hashlib.sha256(proc.stdout).hexdigest()
    folder = SUB / "benchmarks/board"
    folder.mkdir(exist_ok=True)
    text = json.dumps(receipt, indent=2) + "\n"
    path = folder / (now.strftime("%Y-%m-%dT%H%M%S.%fZ") + ".json")
    path.write_text(text)
    (folder / "latest.json").write_text(text)
    print(json.dumps({"receipt": str(path.relative_to(SUB)), "teams": [
        {"team": team["team"], "watch": team.get("watch"), "queue": team.get("queue"),
         "tracks": {track: {"registered": entry.get("registered"), "latest": entry.get("latest")}
                    for track, entry in team["tracks"].items()}} for team in teams]}, indent=2))


if __name__ == "__main__":
    main()
