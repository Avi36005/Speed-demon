# Kryptonite — submission and leaderboard checklist

Repository: https://github.com/Avi36005/Speed-demon
Submission branch: `speed-demon`
Team: **Kryptonite**
Language: **C++**
Tracks: **A · Ingest, B · Index, C · Join**

## Get onto the live leaderboard

1. Every team member completes the individual event registration:
   https://tally.so/r/q4redG
2. Install the Speed Demon Board GitHub App and grant it access to
   `Avi36005/Speed-demon`:
   https://github.com/apps/speed-demon-board
   Select the specific repository when GitHub offers that option. This is the
   organisers' read-only board app, separate from any coding-agent connection.
3. Submit the team/repository form:
   https://tally.so/r/Ar7g9z
   Use the repository, team, language and tracks above; provide the real contact
   email and all 1–4 registered team members. Confirm the form's attestations
   only after they are true. Resubmit this form to change details; latest counts.
4. The board watches the registered repository's `speed-demon` branch. The entry
   has been prepared for that branch. Once the app and team registry are ready,
   check https://speeddemon.dev/board/ for Kryptonite's queue/build/run state.
5. Record actual run IDs, commit IDs and measured results in each `PERF.md` after
   runs exist. Local timings do not establish a leaderboard result.

The current official site and board README specify one dispatch per team at
most every **five minutes**; the form still contains older fifteen-minute text.
Registry processing and queues can add delay. If the repo is pushed, the app has
access, the form is submitted and the team still never appears, contact the
organisers on Discord with the team, repo URL and branch commit.

## What is already in the entry

Each of `track-a`, `track-b` and `track-c` contains `main.cpp`, a pinned offline
`Dockerfile`, `.speed-demon.toml`, `README.md` and `PERF.md`. The repository has
an MIT licence, exact correctness evidence, experiment records and reproduction
commands in `SUBMISSION_REPORT.md`. Each track builds independently.

Submit the **repository URL** through the form. The programs consume the
organisers' `/data` inputs and generate their `/out` files during a run. Generated
datasets, binaries and cached output files do not need to be uploaded. A video
under three minutes is optional and unscored.

## Deadlines in India

- Push the intended final `speed-demon` commit and confirm it was observed before
  **2 November 2026, 20:30 IST** (15:00 UTC code freeze).
- Keep the Board App installed through freeze.
- Make the recorded source public under MIT by **2 November 2026, 21:30 IST**
  (within the one-hour public-source window). Making it public by freeze avoids
  needing the extra hour. This publication task does not change repo visibility.

## History provenance

The original verified development history is preserved, including its real
timestamps, source revisions and measurements. The later file-by-file import
commits package that already completed work for this repository. Their commit
timestamps are the actual import times; they are not separate optimization
experiments or fabricated earlier activity. Only real experiments and real
board run IDs belong in performance claims. A single push containing many commits
does not request a separate leaderboard run for each commit.

## Official instructions

- https://speeddemon.dev/kickstart/
- https://speeddemon.dev/
- https://github.com/hackathon-raptors/speed-demon-2026/blob/main/board/README.md

Registration, form submission and Board App access must be confirmed by the
entrant. Their account state is not inferred from a successful source push.
