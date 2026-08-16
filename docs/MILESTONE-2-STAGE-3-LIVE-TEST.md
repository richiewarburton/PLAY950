# Milestone 2, Stage 3 — eight-voice playback test

Target: PLAY950 0.3.0 in Ableton Live 12.4.3.

`LOOPING.S9` is temporarily assigned to `All(00)` for this test. The mono and
Left/Right assignment limits will return when P9 routing is introduced.

1. Restart Live and rescan PLAY950 so version 0.3.0 replaces version 0.2.0.
2. Hold an eight-note chord containing distinct notes. All eight pitched copies
   of `LOOPING.S9` must sound together and continue looping.
3. While holding those eight notes, play a ninth distinct note. The ninth note
   must sound and the oldest note from the original chord must stop.
4. Release one still-active note. Only that pitch must stop; the other voices
   must continue.
5. Release all notes. Playback must become silent.
6. Repeat the same MIDI sequence twice. Allocation and stealing must sound
   identical on both passes.
7. Confirm playback appears only on Live's `Post FX` (`All(00)`) for this stage.
   Auxiliary outputs must remain silent.
8. Confirm the host exposes no audio input or sidechain bus.

The current stealing rule is explicitly provisional: steal the oldest active
voice. It is deterministic and test-covered, but will be replaced if hardware
measurement proves a different S950 priority rule.

## Acceptance result

Passed all eight checks in Ableton Live 12.4.3 on 2026-08-04. Eight simultaneous
looping voices, ninth-note oldest-voice stealing, selective note release,
deterministic repetition, All-only routing, auxiliary silence and the
playback-only zero-input bus layout all behaved as specified.
