# Milestone 2, Stage 5 — program-driven playback test

Target: PLAY950 0.4.0 in Ableton Live 12.4.3.

`ALWAYS.P9` is loaded automatically for this development test. MIDI notes 36–43
select `ALWAYS01`–`ALWAYS08` on Mono 5; notes 44–49 select `COOL3 01`–`COOL3 06`
on Mono 6. Note 50 belongs to an intentionally blank keygroup.

1. Restart Live and rescan PLAY950 so version 0.4.0 replaces version 0.3.0.
2. Play MIDI notes 36 through 49 individually. Each note must trigger its own
   genuine S9 sample. Notes 35 and 50 must be silent.
3. For notes 36–43, audio must appear simultaneously at `Post FX` (`All(00)`),
   `Mono(05)` and `Right(10)`. Every other auxiliary output must be silent.
4. For notes 44–49, audio must appear simultaneously at `Post FX` (`All(00)`),
   `Mono(06)` and `Right(10)`. Every other auxiliary output must be silent.
5. Hold note 36, then play note 37. Note 37 must replace note 36 because
   `Mono(05)` is monophonic. Releasing the already-replaced note 36 must not stop
   note 37.
6. While note 37 is held, play note 44. Both must sound because Mono 5 and Mono 6
   have independent voices. Playing note 45 must replace only note 44.
7. Release all active notes. Playback must become silent.
8. Confirm the host exposes no audio input or sidechain bus.

This stage does not yet implement Loud velocity layers, crossfades, envelopes,
filters, alternate/reverse playback or editable program selection.
