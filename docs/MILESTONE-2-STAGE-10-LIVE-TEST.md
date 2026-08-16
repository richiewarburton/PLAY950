# Milestone 2, Stage 10 — multi-program IMG browser test

Target host: Ableton Live 12.4.3 on macOS.

PLAY950 0.9.0 replaces the first-program-only IMG policy with a native program
selector. It discovers P9 programs in deterministic filename order, keeps the
extracted content owned by the plug-in, and embeds the complete collection plus
the current selection in VST3 project state. Existing 0.8 single-program Sets
remain readable.

1. Quit and reopen Live, rescan VST3 plug-ins, and add a new PLAY950 instance to
   a MIDI track. Open its editor and confirm the title is `PLAY950 0.9.0`.
2. Click **Open IMG or P9…** and choose
   a disposable copy of a multi-program S950 IMG fixture.
3. Confirm the selector lists `ADG`, `ADGPASTE`, and `ADGPASTE2` in that order,
   with `ADG` initially selected. The status should read
   `Loaded COPYP9.img — ADG`.
4. Select each program in turn. Confirm the status follows the selection and
   each program plays over its mapped MIDI range with no audio interruption,
   editor freeze or crash. Leave `ADGPASTE2` selected.
5. Save the Set, close it, and quit Live.
6. Temporarily rename `COPYP9.img` without changing its contents. Do not rebuild
   the plug-in. Reopen Live and the saved Set.
7. Confirm the selector still contains all three programs and retains
   `ADGPASTE2`. The status should identify it as restored from the Ableton Set.
   Switch among all three programs and confirm they still play without the IMG.
8. Restore the `COPYP9.img` filename after the test.

Acceptance requires all three programs to appear in stable order, switching and
audio to remain responsive, the selected program and complete collection to
survive source-independent Set recall, and no load-time audio interruption or
crash. Record the Live build, macOS build, Set path and result.

## Acceptance run

- Date: 6–7 August 2026.
- Host: Ableton Live 12.4.3 on macOS 26.5.1 (build 25F80).
- Set path: user Stage 10 test Set.
- Result: **PASS**.

The multi-program collection, deterministic ordering, switching, playback and
source-independent Set recall passed. Two status-provenance defects were found:

- A fresh instance in a blank Set incorrectly preloaded `ALWAYS` and described it
  as restored from the Ableton Set.
- After reopening the saved Set, changing the restored program replaced the
  restoration status with `Loaded COPYP9.img — …`.

The fix disables private development-content preloading for normal builds and
retains restored-from-host provenance while switching among restored programs.
Steps 1 and 7 passed on retest on 7 August 2026. Stage 10 is accepted.
