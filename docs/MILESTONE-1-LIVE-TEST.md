# Milestone 1 — Ableton Live host-feasibility test

Target host: Ableton Live 12.4.3 on macOS.

1. Install the built `PLAY950.vst3` bundle in the user VST3 folder and rescan.
2. Add PLAY950 to a MIDI track and play notes. The diagnostic sine must sound.
3. Confirm the host exposes no audio input or sidechain bus for PLAY950.
4. Create audio tracks for `Mono(01)`–`Mono(08)`, `Left(09)` and `Right(10)`.
   `All(00)` is the instrument track's main output and may not appear as a
   separately selectable routing source in Live.
5. Play MIDI. The diagnostic tone must appear on `All(00)`, `Left(09)` and
   `Mono(01)` simultaneously, and nowhere else. This represents a keygroup
   assigned to `Mono(01)` and proves the required duplication behavior.
6. Save, close and reopen the Set; confirm bus routing remains unchanged.

Record the Live build, macOS build, plug-in architecture, visible bus names and
any limitation before Milestone 1 is accepted.

The validated routing model is zero audio inputs and eleven mono output
channels. `All(00)` receives every
voice. `Left(09)` receives direct Left assignments and duplicated Mono(01)–(04)
assignments. `Right(10)` receives direct Right assignments and duplicated
Mono(05)–(08) assignments. Each `Mono(nn)` bus is monophonic.
