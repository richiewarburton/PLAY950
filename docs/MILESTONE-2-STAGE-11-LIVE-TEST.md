# Milestone 2, Stage 11 — native playback-mode test

Target: PLAY950 0.10.0 in Ableton Live 12.4.3 on macOS.

This stage adds native S9 reverse direction and alternating loops plus P9 One
Shot and Constant Pitch behaviour. It does not add envelopes, filters or sample
editing, and it makes no unmeasured envelope-timing assumptions.

## Required genuine fixture

Supply one read-only IMG, or one P9 with linked S9 files, containing:

- a short, recognisable forward non-looping control sample;
- the same audio saved with native S9 reverse direction;
- a clearly audible native S9 alternating/ping-pong loop;
- two P9 keygroups using the same non-looping sample, one normal and one with
  P9 One Shot enabled; and
- one P9 Constant Pitch keygroup spanning at least one octave.

Use sampler-visible names of ten characters or fewer. A spoken count, rising
tone sequence or stepped waveform is preferred because reversal and loop
direction are immediately audible. Keep the original fixture unchanged during
testing.

## Ableton acceptance

1. Quit and reopen Live, rescan VST3 plug-ins, add a fresh PLAY950 instance and
   confirm the editor title is `PLAY950 0.10.0` with no program preloaded.
2. Load the Stage 11 IMG or P9. Confirm the expected program appears and loading
   completes without a freeze, crash or audio spike.
3. Play the forward control and reverse variants. Confirm the reverse variant
   traverses the same content in the opposite direction and stops naturally.
4. Hold the alternating-loop note long enough for several turns. Confirm it
   repeatedly changes direction at both native loop boundaries without a gap,
   duplicated endpoint, click caused by an indexing error or premature stop.
   Release the note and confirm it stops.
5. Press and immediately release the normal non-looping keygroup. Confirm release
   stops it. Repeat with the P9 One Shot keygroup and confirm it ignores the early
   release but stops at the sample's natural end.
6. Play the bottom, middle and top notes of the Constant Pitch keygroup. Confirm
   all produce the same pitch. Confirm an otherwise normal mapped sample still
   transposes chromatically.
7. Confirm each test keygroup retains its native All/Left/Right/Mono routing and
   that the host exposes no audio input or sidechain bus.
8. Save the Set with the Stage 11 program selected, make the source fixture
   temporarily unavailable, reopen the Set and repeat steps 3–6. Restore the
   source fixture afterward.

Acceptance requires correct native direction and loop traversal, correct P9
note-release and pitch behaviour, unchanged routing, and source-independent Set
recall with no freeze, crash or audio spike.

## Acceptance run

- Date: 8 August 2026.
- Host: Ableton Live 12.4.3 on macOS 26.5.1 (build 25F80).
- Fixture: `IMG S9 P9/stage 11/STAGE11.img` and its loose P9/S9 exports.
- Set path: user Stage 11 test Set.
- Result: **PASS**.

All eight acceptance steps passed, including native playback modes, routing,
the playback-only bus layout and source-independent Set recall. The corrected file chooser
sheet also passed its editor-window ordering check.

## Pre-acceptance correction

The first 0.10.0 attempt could report `The processor is busy; try again` when a
new state was queued while Live had not yet processed an audio block. Pending
states now replace older pending states atomically on the control thread. The
genuine `STAGE11.img` extraction and linked-content regression passes with its
one P9 and three S9 samples.

A second attempt exposed an obsolete processor-only guard that still rejected
reverse samples after parsing. Sample preparation now lives in the tested audio
layer, and genuine IMG regressions exercise parsing through preparation. Failed
loads are transactional: the working browser collection is retained and the UI
reports preparation failure rather than claiming the processor is busy or the
IMG contains no programs. Both the legacy `/Music/akaiutil/ALWAYS.img` and the
Stage 11 IMG now have dedicated regressions.

`BREAKS.img` exposed the S950 default `TONE PROGRAM` convention of retaining
`2 SAMPLE` in an unpopulated Loud slot. An absent `2 SAMPLE` is now ignored only
in that Loud slot; Soft references and all other missing samples remain errors.
The genuine three-program `BREAKS.img` regression passes.
