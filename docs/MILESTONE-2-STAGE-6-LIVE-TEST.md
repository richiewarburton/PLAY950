# Milestone 2, Stage 6 — velocity-layer foundation test

Target: PLAY950 0.5.0 in Ableton Live 12.4.3.

`ALWAYS.P9` has a velocity threshold of 128 and no Loud sample names, so every
valid velocity selects its Soft layer. Its velocity-to-loudness value is zero.

1. Restart Live and rescan PLAY950 so version 0.5.0 replaces version 0.4.0.
2. Trigger note 36 repeatedly with MIDI velocities 20, 64, 100 and 127. Every
   hit must play `ALWAYS01`; no velocity may produce silence or select another
   sample.
3. Compare the four hits at `Post FX` with Live's track fader and devices
   unchanged. Their playback level must remain the same; raw MIDI velocity must
   not act as an extra gain control.
4. Confirm note 36 still appears simultaneously at `Post FX` (`All(00)`),
   `Mono(05)` and `Right(10)`, with every other auxiliary output silent.
5. Confirm note 44 still appears simultaneously at `Post FX` (`All(00)`),
   `Mono(06)` and `Right(10)`, with every other auxiliary output silent.
6. Confirm the host exposes no audio input or sidechain bus.

Soft/Loud threshold selection and independent layer tuning are covered by the
automated engine test. Audible Loud-layer acceptance needs a genuine P9 containing
both layer names. Velocity crossfade and velocity-to-loudness remain deferred.
