# Milestone 2, Stage 13 — EDIT950 workflow bridge

Target: PLAY950 0.12.0 in Ableton Live 12.4.3 on macOS.

## Fixture requirements

Use only the three files in `IMG S9 P9/WORKFLOW BRIDGE FIXTURE`. ORIGINAL and
UPDATED are reference images; ACTIVE is the disposable source PLAY950 loads.
EDIT950 must be installed at `/Applications/EDIT950.app`
with preferred bundle identifier `com.e45recordings.EDIT950` while still
recognizing the legacy `com.local.AKAIImageManager` identifier.

Before testing, copy `WORKFLOW-ORIGINAL.img` over `WORKFLOW-ACTIVE.img`.

## Ableton acceptance

1. Quit and reopen Live, rescan VST3 plug-ins, add a fresh PLAY950 and confirm
   `PLAY950 0.12.0`. With no content loaded, Reload IMG and Open in EDIT950 must be
   disabled.

2. Open `WORKFLOW-ACTIVE.img`. Confirm 44 programs appear, loading succeeds,
   Reload IMG and Open in EDIT950 become enabled, and ACTIVE appears at the top of
   Recent IMGs.

3. Select `FLESLOW` and play MIDI 60. Without closing Live, copy
   `WORKFLOW-UPDATED.img` over `WORKFLOW-ACTIVE.img`, then click Reload IMG.
   Confirm there are now 45 programs, `FLESLOW` remains selected by filename,
   the status begins `Reloaded WORKFLOW-ACTIVE.img`, and playback continues with
   the updated collection. Select `ZZRELOAD` and confirm it plays.

4. Select `FLESLOW` again. Temporarily rename `WORKFLOW-ACTIVE.img` to
   `WORKFLOW-ACTIVE.img.unavailable`, then click Reload IMG. Confirm the status
   reports reload failure and explicitly says previous content was retained.
   Confirm all 45 embedded programs remain available and `FLESLOW` still plays.

5. Close and reopen the plug-in editor while ACTIVE remains unavailable. Open
   Recent IMGs and confirm ACTIVE is marked missing and cannot be selected. Choose
   Remove Missing Items and confirm it disappears. Restore the ACTIVE filename by
   renaming the unavailable file back.

6. Load ACTIVE again and confirm it returns to the top of Recent IMGs. Load
   ORIGINAL, then load ACTIVE once more; confirm both appear once only, ordered by
   most recent successful load. Select each recent image and confirm it loads
   without opening a file chooser.

7. With ACTIVE loaded, click Open in EDIT950. Confirm EDIT950 launches
   or comes forward and opens exactly `WORKFLOW-ACTIVE.img`. PLAY950 must remain
   responsive and continue playing.

8. Return to Live, select `FLESLOW`, save the Set, close it and quit Live. Rename
   ACTIVE unavailable again, reopen Live and the Set. Confirm all programs and
   `FLESLOW` restore from the Ableton Set and play without the source image.
   Reload IMG must fail transactionally without replacing the embedded content.

9. Restore ACTIVE, click Reload IMG, and confirm successful reload retains
   `FLESLOW`. Restore ACTIVE to ORIGINAL after the test.

10. Regression-check static filter, long amplitude release, program switching,
    All/Mono/Left/Right routing and the file chooser's sheet ordering. Confirm
    the host exposes no audio input or sidechain bus, with no freeze, crash,
    busy error or audio spike.

## Pass rule

Pass when reload is transactional, filename selection survives successful
reload, recents are deduplicated and removable when missing, the editor handoff
opens the exact IMG, embedded recall remains authoritative without the source,
and existing playback/routing behavior is unchanged.

## Acceptance run

- Date: 9 August 2026.
- Host: Ableton Live 12.4.3 on macOS.
- Result: **PASS.**

Reload, recents, missing-item cleanup, exact EDIT950 handoff, transactional failure,
embedded recall, filter/envelope playback, program switching, output routing and
file-chooser ordering all passed. The first UI used one cramped four-control row;
the correction uses two full-width control rows with readable labels.

An older Set restored `OUTPUTCHECK.img` without enabling Reload or Open in EDIT950.
This is expected for schema-v1/v2 state, which predates source-path metadata.
The corrected UI explains that the IMG must be opened once and the Set resaved;
schema-v3 Sets retain the path. PLAY950 deliberately does not guess a disk path
from a source filename. The subsequent two-row layout and legacy-state guidance
recheck passed.
