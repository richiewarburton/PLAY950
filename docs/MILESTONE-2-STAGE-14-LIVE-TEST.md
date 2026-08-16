# Milestone 2, Stage 14 — loudness and velocity response

Target: PLAY950 0.12.2 in Ableton Live 12 on macOS.

## Fixture

Use `PSL9013_ElectronicDrums.img`. Its `DRUM-1` kick uses `BASS DRUM.S9`
(sample loudness +5), P9 Soft loudness +20 and velocity-to-loudness 10. The
decoded S9 peak is approximately -5.25 dBFS. Older PLAY950 builds incorrectly
read +20 as +20 dB and could therefore render the single kick above +14 dBFS.

## Ableton acceptance

1. Quit and reopen Live, rescan VST3 plug-ins, add a fresh PLAY950 and confirm
   the editor displays `PLAY950 0.12.2`.
2. Load `PSL9013_ElectronicDrums.img`, select `DRUM-1`, and play the kick at MIDI
   velocities 20, 64, 100 and 127 with the track/device gains unchanged.
3. Confirm every kick remains below 0 dBFS. At velocity 127 the bounded digital
   model predicts no more than about -1.73 dBFS before pitch interpolation,
   filtering and the amplitude envelope; it must never reproduce the former
   +14 dBFS spike.
4. Confirm the kick changes only gently across those velocities, consistent with
   its sensitivity of 10. Play the `DRUM-1` snare (sensitivity 50, loudness +00)
   and confirm it has a clearly wider velocity-dependent level range.
5. Select `DRUM-8`. Its keygroups use velocity sensitivity 00; repeated notes at
   different velocities must retain the same level while their distinct
   loudness offsets continue to balance the samples.
6. Save the Set, close and reopen it, and confirm the selected program, loudness
   response, routing and source-independent playback all survive recall.

## Pass rule

Pass when the PSL9013 kick cannot reproduce the former overload, velocity 00/10/
50 settings have progressively stronger audible response, zero sensitivity stays
level-independent, and Set recall is unchanged.

This acceptance checks the manual-defined endpoints and the real bug fixture.
Exact intermediate correspondence to a physical S900/S950 remains a separate
hardware differential-calibration task.
