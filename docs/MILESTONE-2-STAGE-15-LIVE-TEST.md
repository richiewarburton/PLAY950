# Milestone 2 Stage 15 — multitimbral MIDI acceptance

Target: PLAY950 0.13.0 in Renoise on macOS.

## Purpose

Confirm that one PLAY950 instance reproduces the S950's Basic-Channel-relative
P9 keygroup routing, including overlapping note spans, channel-isolated note
release and pitch bend.

## Setup

- A P9 with at least two audible keygroups whose note ranges overlap and whose
  MIDI channel offsets differ, ideally offsets 0 and 1.
- One Renoise instrument containing PLAY950.
- Two Plugin Alias instruments targeting that PLAY950 instance, assigned to
  MIDI channels 1 and 2.
- Dedicated pattern tracks using the two alias instrument numbers.

## Acceptance

1. Open PLAY950 and confirm the editor displays `PLAY950 0.13.0` and the MIDI
   controls `RECEIVE`, `BASIC CH` and `BEND`.
2. Load the multichannel P9. Leave Receive on **Omni** and confirm ordinary
   channel-agnostic playback remains available.
3. Select **KG Channels**, set Basic Channel to 1, and play the same overlapping
   note through alias channels 1 and 2. Confirm each alias triggers only its
   intended keygroup.
4. Send the note on both aliases together, then stop only channel 1. Confirm the
   channel-2 voice remains active until its own note-off.
5. Hold notes on both aliases and apply full-up pitch bend only to channel 1.
   Confirm channel 1 follows the selected bend range while channel 2 remains at
   its original pitch.
6. Change Basic Channel to 16. Confirm offset 1 wraps to MIDI channel 1.
7. Restore Basic Channel 1, save the song, close Renoise, reopen it, and confirm
   the loaded program, **KG Channels** mode, Basic Channel and bend range recall.
8. Confirm all aliases still feed one PLAY950 instance and share its global
   eight-voice limit and output assignments.

## Automated prerequisites

Before live acceptance, run:

```sh
cmake --build build-cli --config Release --target \
  play950_p9_tests play950_program_audio_tests play950_project_state_tests PLAY950
./build-cli/play950_p9_tests
./build-cli/play950_program_audio_tests
./build-cli/play950_project_state_tests
ctest --test-dir build-cli -C Release --output-on-failure
```
