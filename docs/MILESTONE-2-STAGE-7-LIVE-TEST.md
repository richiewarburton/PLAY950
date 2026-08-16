# Milestone 2, Stage 7 — AKAI Util IMG playback test

Target: PLAY950 0.6.0 in Ableton Live 12.4.3.

This build obtains `ALWAYS.P9` and its fourteen linked samples from the genuine
`ALWAYS.img` using AKAI Util 4.6.7 in read-only mode. The automated regression has
already proved that all fifteen exports match the loose fixtures byte-for-byte.

1. Restart Live and rescan PLAY950 so version 0.6.0 replaces version 0.5.0.
2. Play notes 36 through 49 individually. All fourteen program samples must play;
   notes 35 and 50 must remain silent.
3. Confirm note 36 appears at `Post FX` (`All(00)`), `Mono(05)` and `Right(10)`
   only.
4. Confirm note 44 appears at `Post FX` (`All(00)`), `Mono(06)` and `Right(10)`
   only.
5. Hold note 36 and play note 37. Note 37 must replace note 36 on monophonic
   `Mono(05)`. Note 44 must still play simultaneously on `Mono(06)`.
6. Confirm the host exposes no audio input or sidechain bus.

This stage establishes reliable read-only IMG extraction and playback. It does
not yet expose an image chooser in the plugin interface or embed extracted image
content in Ableton project state.
