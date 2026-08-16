# PLAY950 project status

Last updated: 16 August 2026

PLAY950 0.13.3 is a working macOS VST3 development project. It is not yet a
signed, notarized public plug-in release.

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
- Check GitHub when the editor opens and show a release-page button only when a
  newer PLAY950 version is available.
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

## Before a packaged public release

- Complete the remaining listening checks in the supported DAW hosts.
- Sign with a Developer ID Application certificate.
- Notarize and staple the exact release bundle.
- Test the packaged ZIP on clean Apple-silicon and Intel Macs.

The release process is documented in [RELEASE.md](RELEASE.md).
