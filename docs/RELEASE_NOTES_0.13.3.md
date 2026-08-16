# PLAY950 0.13.3

PLAY950 0.13.3 completes the current multitimbral playback, editor and
self-contained IMG-loading work.

## Editor and workflow

- Increases editor typography by 25 percent, including buttons, menus, section
  headings, program identity and status text.
- Adds an adaptive light palette while retaining the established dark design.
- Adds a persistent **Theme** menu with System, Light and Dark choices. System
  is the default and follows the current macOS appearance.
- Checks GitHub when the editor opens and shows a compact release-page button
  only when a newer version is available. PLAY950 does not download or install
  updates automatically.
- Opens FIND950 for library browsing and the current source IMG in EDIT950.

## Native content and recall

- Bundles a Universal AKAI Util 4.6.7 helper for read-only IMG extraction; no
  separate download, path selection, permission command or EDIT950 installation
  is required.
- Loads every P9 from a multi-program IMG, switches programs without reopening
  the disk and retains the selection when Reload Source succeeds.
- Loads loose P9/S9 content natively and embeds the current program and samples
  in versioned DAW project state for source-independent recall.

## MIDI and outputs

- Adds Omni or P9 keygroup-channel receive modes with a selectable S950 Basic
  MIDI Channel.
- Keeps pitch bend and note release isolated per incoming MIDI channel for
  multitimbral use, including Renoise Plugin Alias workflows.
- Retains the global eight-voice pool and S950-style All (00), Mono (01–08),
  Left (09) and Right (10) outputs.

## Verification

- Universal `arm64`/`x86_64` Release VST3 build succeeded.
- All 7 CTest targets passed.
- Steinberg validator passed all 47 tests.
- Strict code-signature verification passed.
- The real Steinberg editor host rendered System/Dark and forced Light modes
  with the enlarged typography, complete labels and contrasting adaptive
  controls.
