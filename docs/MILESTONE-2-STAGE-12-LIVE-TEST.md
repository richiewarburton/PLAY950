# Milestone 2, Stage 12 — approximate S950 filter/envelope test

Target: PLAY950 0.11.0 in Ableton Live 12.4.3 on macOS.

This stage adds a deliberately approximate, hardware-informed playback filter,
amplitude envelope and VCF envelope. The target is acceptable translation
between the physical S950 and PLAY950, not component-level analogue emulation.

## Fixture requirements

- IMG: `IMG S9 P9/CURRENT FILTER ENVELOPE FIXTURE/TRUE950-FILTER-ENVELOPE-CURRENT.img`
- One MIDI track with PLAY950 and monitoring through `All(00)`.
- MIDI notes 48, 54, 60, 66 and 72; a velocity-selectable controller or clips
  at velocities 20, 64, 100 and 127.
- No source recording is required. Keep the physical-S950 reference WAV only for
  optional A/B comparison.

## Ableton acceptance

1. Quit and reopen Live, rescan VST3 plug-ins, add a fresh PLAY950 instance and
   confirm the editor title is `PLAY950 0.11.0` with no program preloaded.

2. Load the current fixture IMG. Confirm all 44 programs appear and `C3SINE` is
   initially selected. Loading must complete without a busy error, freeze,
   crash or audio spike.

3. Select `FLNC00`, `FLNC20`, `FLNC40`, `FLNC60`, `FLNC80` and `FLNC99` in that
   order. For each, play MIDI 60 at velocity 100 for two seconds. Confirm overall
   brightness increases across the range, with 0/20 relatively close, the most
   useful movement through 40–80, and 80/99 both near-open.

4. Repeat step 3 with `FLSC00/20/40/60/80/99`. Confirm the saw retains its pitch
   while harmonic brightness follows the filter value.

5. Select `FLTRACK`. At velocity 100, play MIDI 48, 54, 60, 66 and 72 for two
   seconds each. Confirm normal chromatic pitch transposition and a monotonic
   brightness increase. There must be no octave wrapping or Constant Pitch
   behavior.

6. At MIDI 60/velocity 100, hold `FLEN50`, `FLEZ00` and `FLEP50` for six seconds
   each. Confirm `FLEN50` starts relatively closed and audibly opens toward its
   high base cutoff; `FLEZ00` remains static at its half-closed base cutoff; and
   `FLEP50` starts relatively open and audibly closes toward its low base cutoff.
   The two modulated programs must move in opposite directions without clipping
   against a fully open or fully closed filter for their entire useful travel.

7. At MIDI 60/velocity 100, hold `FLEFAST` for four seconds, `FLEMID` for eight
   seconds and `FLESLOW` for fifteen seconds. Confirm progressively slower filter
   movement without stepping, clicks, premature stops or instability.

8. Select `FLVEL` and play MIDI 60 for three seconds at velocities 20, 64 and
   127. Confirm progressively brighter playback without an unintended direct
   velocity-to-volume change.

9. Test amplitude Attack using `ENVA01`, `ENVA50`, `ENVA99`; hold MIDI 60 at
   velocity 100 for 4, 8 and 15 seconds respectively. Confirm progressively slower
   onset. No note may click or remain silent indefinitely.

10. Hold MIDI 60/velocity 100 for ten seconds on `ENVD00`, `ENVD50`, `ENVD99`,
    then for eight seconds on `ENVS00`, `ENVS50`, `ENVS99`. Confirm progressively
    slower decay and clearly increasing sustain level; `ENVS00` should settle
    effectively silent while the note remains held.

11. Hold MIDI 60/velocity 100 for four seconds on `ENVR00`, `ENVR50`, `ENVR99`,
    then release. Confirm immediate, medium and long release tails respectively;
    each voice must eventually stop naturally.

12. Regression-check `ENVONESH`: press and immediately release MIDI 60. Confirm
    it ignores the early release. Switch to `C3SINE`, hold MIDI 60 and confirm its
    full-file loop is clean and note-off stops it.

13. Confirm `All(00)` and the existing Mono/Left/Right routing remain unchanged.
    Program switching must not freeze, crash or leave voices from the previous
    program sounding.
PASS

14. Save the Set with `FLESLOW` selected, close the Set, quit Live, make the
    source IMG temporarily unavailable, reopen the Set and repeat steps 6, 7 and
    11. Confirm embedded recall retains the selected program and all filter and
    envelope behavior. Restore the IMG afterward.

## Pass rule

Pass when every control changes playback in the expected direction, timing is
musically plausible relative to the hardware recording, existing playback and
routing remain intact, and source-independent Set recall succeeds. Small tonal
or timing differences from the physical S950 are acceptable at this stage.

## Result

PASS — 9 August 2026, Ableton Live 12.4.3. All playback, modulation, envelope,
routing and embedded-recall checks passed. Step 6 initially exposed inadequate
fixture headroom; after correcting `FLEN50` to Filter 99 and `FLEP50` to Filter
0, the opposite-direction VCF movements passed clearly without a plug-in change.
