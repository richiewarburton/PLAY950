# PLAY950 project status

Last updated: 22 August 2026

PLAY950 0.13.5 is the current public macOS VST3 community release. The packaged
plug-in is Universal and ad-hoc signed; it is not Developer ID signed or
notarized.

## Working now

- Load a standard S900/S950 IMG read-only and choose any native P9 program.
- Load a loose P9 with its linked S9 samples.
- Play keygroups, Soft/Loud layers, tuning, forward/reverse playback, loops,
  envelopes, filtering and the supported native output assignments.
- Use Omni mode or honour P9 keygroup MIDI-channel offsets relative to the
  selected S950 Basic Channel.
- Save the current program and samples with the DAW project so recall does not
  depend on the original source path.
- Open the exact selected P9 in EDIT950 and audition validated in-memory
  revisions in only the initiating plug-in instance.
- Reload an externally changed or newly saved IMG while preserving the selected
  program by filename when possible.
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

## Remaining distribution improvements

- Automatically rebase the live-edit baseline after a verified EDIT950 Save;
  tracked in [issue #5](https://github.com/richiewarburton/PLAY950/issues/5).
- Sign with a Developer ID Application certificate.
- Notarize and staple the exact release bundle.
- Test the packaged ZIP on clean Apple-silicon and Intel Macs.

The release process is documented in [RELEASE.md](RELEASE.md).
