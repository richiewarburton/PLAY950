# Milestone 2, Stage 9 — plug-in IMG/P9 chooser test

Target host: Ableton Live 12.4.3 on macOS.

PLAY950 0.8.0 adds a native macOS editor and asynchronous **Open IMG or P9…**
workflow. File access, read-only AKAI Util extraction, native parsing and content
preparation occur away from the audio thread. The completed program is adopted at
an audio-block boundary and included in VST3 project state.

1. Quit and reopen Live, rescan VST3 plug-ins, and add a new PLAY950 instance to
   a MIDI track.
2. Open the plug-in editor. Confirm the `PLAY950 0.8.0` title, chooser explanation,
   **Open IMG or P9…** button and status line are visible.
3. Click **Open IMG or P9…** and choose `IMG S9 P9/ALWAYS.img`.
4. Wait for `Loaded ALWAYS.img`. Play notes 36–49 and confirm all fourteen
   samples plus their accepted Mono, Left/Right and All routes.
5. Save the Set, close it, and quit Live.
6. Temporarily rename both `IMG S9 P9/ALWAYS.img` and
   `build-cli/development-image` without altering their contents. Reopen Live and
   the saved Set without rebuilding.
7. Play notes 36–49. All fourteen samples and routes must remain identical,
   proving that chooser-loaded content was embedded in project state.
8. Restore both names. Open the chooser again, select `IMG S9 P9/ALWAYS.P9`, and
   confirm `Loaded ALWAYS.P9` plus the same notes and routes.

Acceptance requires the editor and chooser to remain responsive, both IMG and
direct P9 loading to pass, no audio interruption or crash during loading, and
successful Set recall while both original sources are unavailable. Record the
Live build, macOS build, Set path and result.

## First acceptance run

- Date: 5 August 2026.
- Host: Ableton Live 12.4.3 on macOS 26.5.1 (build 25F80).
- Result: **PARTIAL PASS** pending UI-status retest.
- Native editor, IMG loading, notes 36–49, all accepted routes and Set recall with
  both sources unavailable passed.
- Direct `ALWAYS.P9` selection retained correct notes and routes, but the editor
  showed its default status after Ableton recreated the view. Live did not expose
  the plug-in version at the location assumed by the original step 1.
- The follow-up build stores status in the controller so recreated editor views
  retain `Loaded <filename>`, and displays version 0.8.0 in the editor title.

## Follow-up acceptance

- Date: 5 August 2026.
- Result: **PASS**.
- The editor displayed `PLAY950 0.8.0`.
- Direct `ALWAYS.P9` selection displayed `Loaded ALWAYS.P9`.
- Closing and reopening the editor retained the loaded-file status.
- Combined with the first run's passed IMG loading, playback/routing and
  source-independent Set recall, Milestone 2 Stage 9 is accepted.
