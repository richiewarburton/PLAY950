# Native P9 program format — read-only implementation

PLAY950's `play950_formats` library reads the 38-byte S950 P9 program header and
its fixed 70-byte keygroup records without using AKAI Util at runtime.

The parser retains every raw header and keygroup byte while exposing the fields
needed by the player: program name and positional crossfade; key and velocity
ranges; amplitude and VCF envelopes; velocity sensitivities; known flag bits;
MIDI channel; output assignment; Soft and Loud sample names, tuning, filter and
loudness; VCF amount and velocity-crossfade point.

Physical-S950 differential testing established that Key-filter is keygroup byte
`0x08`; LFO Depth is the independent byte at `0x16`. PLAY950 parses Key-filter
from `0x08` and deliberately does not reuse either `0x11` or `0x16` for it.

Output bytes translate as follows:

- `0xFF` → All(00)
- `0x00`…`0x07` → Mono(01)…Mono(08)
- `0x08` → Left(09)
- `0x09` → Right(10)

Unknown output values and undocumented flag bits remain available in the raw
records. Tuning is retained exactly in signed 1/16-semitone units; display
transpose and Fine values use the S950 nearest-semitone convention.

## MIDI channel offsets

Keygroup byte `0x14` is retained as the native 0–15 MIDI-channel offset. The P9
does not contain the S950's machine-global Omni or Basic MIDI Channel settings,
so PLAY950 exposes those settings in its MIDI panel and stores them with the DAW
project. In **Omni** mode, keygroup selection remains channel-agnostic. In
**KG Channels** mode, the effective zero-based event channel is:

```text
(Basic MIDI Channel - 1 + keygroup offset) modulo 16
```

Note-on, note-off and pitch bend retain their incoming channel. This permits
overlapping key ranges on different channels to share one PLAY950 instance and
one global eight-voice pool, matching a single multitimbral S950 rather than
creating a sampler instance per channel.

Sample resolution uses exact case-insensitive ten-character internal S9 names.
Ambiguous links are errors. Missing links remain unresolved so only that layer
is silent; they do not reject the rest of a valid program. Genuine-file
regressions exercise multi-keygroup programs and linked native S9 files without
distributing privately owned sampler content.

The S900/S950 default program commonly leaves `2 SAMPLE` in an unused sample
slot. It is a placeholder rather than a sample name. PLAY950 therefore treats a
case-insensitive `2 SAMPLE` reference in either layer as empty.

## Loudness and velocity

P9 Soft/Loud values and the linked S9 loudness value are signed native control
offsets, not decibels. Playback adds the two offsets and clamps the result to the
native -50...+50 range. That range is mapped linearly around unity to a bounded
0...2 gain. The P9 velocity-to-loudness value controls how much of the remaining
range is subtracted at lower MIDI velocities: 00 is level-independent, 99 is the
strongest response, and a +50 loudness offset removes velocity dynamics as
described by the S950 manual.

This is a bounded, manual-anchored transfer model. Its endpoints and interaction
are deterministic and prevent native values such as +20 from being mistaken for
+20 dB. Exact intermediate analogue levels remain candidates for differential
calibration against physical hardware.
