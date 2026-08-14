# PLAY950 project status

Last updated: 14 August 2026

PLAY950 0.13.1 is available as a public Universal macOS VST3 community build on
[GitHub Releases](https://github.com/richiewarburton/PLAY950/releases/tag/v0.13.1).
The published build is ad-hoc signed and is not Apple-notarized.

## Working now

- Load a standard S900/S950 IMG read-only and choose any native P9 program.
- Load a loose P9 with its linked S9 samples.
- Play keygroups, Soft/Loud layers, tuning, forward/reverse playback, loops,
  envelopes, filtering and the supported native output assignments.
- Use Omni mode or honour P9 keygroup MIDI-channel offsets relative to the
  selected S950 Basic Channel.
- Save the current program and samples with the DAW project so recall does not
  depend on the original source path.
- Reload an edited IMG, choose from recent images and open the source in
  EDIT950.
- Run natively on Apple silicon and Intel without Rosetta.

PLAY950 has one MIDI input, eleven mono audio outputs and no audio input. It
does not record, sample, monitor incoming audio or modify an IMG.

## Validation

The public synthetic suites cover P9/S9 parsing, voice allocation, MIDI-channel
behaviour, playback modes, envelopes, filtering, project-state migration and
read-only image workflow rules.

Separate private-fixture checks have covered real IMG extraction, multi-program
selection, linked native content, program switching, source-independent Set
recall and byte-equivalence of the bundled Universal AKAI Util helper. Private
sampler files and their filesystem locations are not part of this repository.

The Universal VST3 has also passed the Steinberg validator and native Ableton
Live smoke tests on Apple silicon.

## Future Developer ID and notarization work

- Continue listening checks in additional supported DAW hosts.
- Sign with a Developer ID Application certificate.
- Notarize and staple the exact release bundle.
- Repeat clean-machine acceptance for the notarized package on Apple-silicon
  and Intel Macs.

These are hardening steps for a future notarized distribution, not blockers to
the existing public 0.13.1 community release. The release process is documented
in [RELEASE.md](RELEASE.md).
