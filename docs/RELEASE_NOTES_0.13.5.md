# PLAY950 0.13.5

PLAY950 0.13.5 adds private, low-latency P9 audition sessions with EDIT950 while
keeping PLAY950 read-only and the source IMG unchanged until explicit Save.

## Exact-program EDIT950 handoff

- **Edit This IMG** opens the exact selected P9 rather than only handing off the
  source IMG.
- A short-lived, versioned request carries the source path, P9 filename, current
  program, source baseline, revision and a stable random identifier for this
  plug-in instance.
- Requests are written atomically and removed on success or launch failure.
- New source loads, reloads and target changes invalidate the previous session.

## Live P9 audition

- Accept validated, monotonic P9-only revisions from the matching EDIT950
  session and acknowledge success or a specific sync error.
- Reuse the already prepared S9 sample data instead of copying samples,
  extracting the IMG again or rebuilding unrelated programs.
- Preserve the current MIDI Receive, Basic Channel and pitch-bend settings while
  replacing only the selected program's keygroups.
- Persist the currently auditioned P9 with host state so DAW project recall
  matches the active sound.
- Report connection, disconnection and revision state through the local
  versioned notification protocol.

## Reload Source

Reload Source remains intentionally available for external IMG changes,
added/deleted/renamed P9 or S9 content, changed sample data and recovery from a
stale or disconnected session. Version 0.13.5 still requires it after closing a
session that saved its P9 before starting another edit session. Automatic
same-session rebase is tracked in
[issue #5](https://github.com/richiewarburton/PLAY950/issues/5).

## Build and verification

- Core formats, state, workflow and audio tests can build without configuring
  the VST3 SDK; all 7 CTest targets pass.
- A one-million-sample, 99-keygroup P9 update shares prepared sample storage and
  completes within the 100 ms acceptance boundary.
- The Universal `arm64`/`x86_64` Release plug-in and native Debug plug-in build
  successfully with the bundled Universal AKAI Util helper.
- Steinberg validator passes all 47 tests and strict code-signature verification
  passes.
- A real editor-host session opened a six-keygroup P9 in EDIT950, rejected an
  invalid revision, recovered to Auditioned on the next valid revision and left
  the source IMG byte-identical.
