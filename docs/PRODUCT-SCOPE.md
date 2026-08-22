# PLAY950 product scope

Reframed: 8 August 2026

## Product role

PLAY950 is a macOS VST3 playback companion to EDIT950.
It provides an offline replacement for the playable parts of an S950 when the
hardware is unavailable, while EDIT950 remains the dedicated native
content editor.

The two applications divide responsibility as follows:

- **EDIT950:** create, inspect and edit IMG, P9 and S9 content.
- **PLAY950:** load that content, play it accurately in a DAW, reproduce the S950
  playback filter, route its outputs, and preserve everything
  required for reliable Ableton Set recall.

## PLAY950 version-one scope

### Playback

- One MIDI event input and no audio input buses.
- Read-only IMG and direct P9/S9-compatible loading.
- Multi-program browsing and instant program selection.
- Native keygroups, layers, tuning, playback modes, polyphony and routing.
- Omni or Basic-Channel-relative P9 keygroup MIDI reception, with channel-local
  note release and pitch bend for one-instance multitimbral sequencing.
- S950 filter behaviour, including the understood P9 filter modulation fields.
- Embedded source-independent Ableton project recall.

### EDIT950 workflow bridge

- **Reload Source** deliberately re-reads the complete source on a background
  queue after changes made by another session or tool, content additions,
  deletions or renames, or a stale/disconnected session. It preserves the
  selected program by P9 filename where possible and changes nothing if reload
  fails.
- **Recent IMGs** provides quick access to a small per-user list of successfully
  loaded images. Missing entries are clearly identified and removable.
- **Open in EDIT950** writes a short-lived, versioned session request containing
  the exact selected P9, source IMG path and a stable random identifier for this
  plug-in instance. EDIT950 returns validated P9-only revisions through the
  local live-audition channel; prepared S9 samples are shared rather than copied
  or parsed again. The IMG is never changed by PLAY950.
- Source paths are workflow conveniences, not playback dependencies. Embedded
  state remains authoritative when an Ableton Set is reopened without its IMG.

## Explicitly delegated to EDIT950

PLAY950 will not duplicate:

- audio recording, live input monitoring or sampling;
- P9/keygroup editing;
- waveform, marker or loop editing;
- IMG content management or write-back;
- general S9/P9 metadata editing;
- destructive sample editing; or
- a second full program/sample editor interface.

## Revised delivery sequence

1. **Stage 12 — filter/envelope approximation:** fit the supplied S950 capture
   and implement base filter, modulation and amplitude/VCF envelopes.
2. **Workflow bridge:** current-source tracking, Reload Source, Recent IMGs and Open
   in EDIT950.
3. **Hardening and release:** compatibility testing, documentation, packaging,
   signing and notarization.

At every stage, audio-thread safety, Universal macOS support, deterministic tests,
Steinberg validation and Ableton acceptance remain mandatory.
