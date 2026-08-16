# Milestone 2, Stage 2 — genuine S9 playback test

Target: PLAY950 0.2.0 in Ableton Live 12.4.3.

1. Restart Live and rescan PLAY950 so version 0.2.0 replaces the earlier spike.
2. Add PLAY950 to a MIDI track and play MIDI note 59 (B3 in PLAY950/S950 MIDI
   numbering; Live may display octave names differently). `LOOPING.S9` must play
   at its original 48 kHz/root-pitch speed.
3. Hold the note beyond the initial sample section. Playback must repeat the
   native loop from sample 11,000 to 27,693 for as long as the note is held.
4. Release the note. The looping voice must stop.
5. Play notes 47, 59 and 71. They must track one octave below, original pitch,
   and one octave above respectively.
6. Confirm the voice appears simultaneously at Live's `Post FX` (`All(00)`),
   `Mono(01)` and `Left(09)`, and is silent on every other auxiliary output.
7. Confirm the host exposes no audio input or sidechain bus.

This stage intentionally has one voice, one development fixture and linear
interpolation. It does not yet provide a file picker, embedded project state,
alternate/reverse playback, envelopes or the final eight-voice allocator.

## Acceptance result

Passed all seven checks in Ableton Live 12.4.3 on 2026-08-04, including native
pitch and looping, note-off, octave tracking, duplicated hardware-style routing,
silence on unused outputs, and the playback-only zero-input bus layout.
