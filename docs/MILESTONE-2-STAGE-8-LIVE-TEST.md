# Milestone 2, Stage 8 — embedded project-state recall test

Target host: Ableton Live 12.4.3 on macOS.

The installed PLAY950 0.7.0 development build embeds the loaded `ALWAYS.P9` and
all fourteen linked S9 files in VST3 component state. This test proves that a
saved Set recalls its content rather than silently reloading the build fixture.

1. Rescan VST3 plug-ins and add PLAY950 to a MIDI track.
2. Play notes 36 through 49 and confirm all fourteen program samples sound on
   their previously accepted Mono, Left/Right and All output routes.
3. Save the Set, close it, and quit Live.
4. Temporarily rename `build-cli/development-image` without changing its contents.
5. Reopen Live and the saved Set. Do not rebuild the plug-in first.
6. Play notes 36 through 49 again. All fourteen samples and output routes must be
   identical to step 2.
7. Restore the `build-cli/development-image` directory name after the test.

Acceptance requires successful recall in step 6 with the extracted development
files unavailable. Record the Live build, macOS build, saved Set path and result.

## Acceptance record

- Date: 5 August 2026.
- Result: **PASS**.
- Host: Ableton Live 12.4.3.
- System: macOS 26.5.1 (build 25F80).
- Plug-in: PLAY950 0.7.0 Universal arm64/x86_64 VST3.
- Set: `Test Ableton set Project/Test Ableton set.als`.
- Before save, notes 36–49 played all fourteen samples through their accepted
  Mono, Left/Right and All output routes.
- After save/quit, `build-cli/development-image` was renamed and therefore
  unavailable to the plug-in. Reopening the Set restored all fourteen samples
  and identical routing from embedded VST3 component state.
- `build-cli/development-image` was restored after the test and its directory is
  present under its original name.
