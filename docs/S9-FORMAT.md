# Native S9 sample format — implemented subset

PLAY950 currently reads uncompressed S900/S950 S9 samples. Parsing is isolated
in `play950_formats` and performs no file access, allocation or mutation on the
audio thread.

## Header

The native header is 60 bytes. The current parser exposes the ten-byte internal
name, sample count and rate, nominal pitch in 1/16-semitone units, loudness,
playback mode, start/end and loop length, sample type, and direction. It retains
all 60 raw bytes so undocumented and sampler-maintained fields can be preserved
by future edited serialization.

All multi-byte numbers in this header are little-endian. Playback modes use the
native `O`, `L`, and `A` byte values; directions use `N` and `R`.

## Uncompressed 12-bit payload

The payload contains `3 * ceil(sampleCount / 2)` bytes. It is not ordinary
interleaved 16-bit PCM and is not arranged as adjacent three-byte sample pairs.
The first half of the samples stores a shared-nibble byte plus an upper-eight-bit
byte for each sample. The second half stores its upper-eight-bit plane after the
first-half pairs and uses the unused low nibble in each first-half shared byte.

Decoded samples are retained as signed integers in the exact 12-bit range
−2048…2047. Conversion to host floating point belongs to the playback engine.
Program playback combines the signed S9 loudness offset with its selected P9
layer offset before applying velocity-to-loudness response.

## Validation sources

- EDIT950's tested field offsets and genuine-file results.
- AKAI Util's independently maintained S900 conversion implementation as a
  behavioral reference; it is not linked, bundled or copied into PLAY950.
- Fourteen private genuine S9 fixtures, exercised locally without committing
  their content.

Compressed S9 payloads are deliberately rejected by this first implementation.
They will be added behind an explicit format distinction once IMG directory
metadata and compressed fixtures are available to identify them reliably.
