# PLAY950 0.13.1 installation

PLAY950 requires macOS 14 or later and a VST3 host such as Ableton Live 12.

Download the public Universal macOS package from the
[PLAY950 GitHub Releases page](https://github.com/richiewarburton/PLAY950/releases/latest).
The community build is ad-hoc signed and is not Apple-notarized.

1. Quit Ableton Live.
2. Copy `PLAY950.vst3` to `~/Library/Audio/Plug-Ins/VST3/`.
3. Reopen Live and rescan VST3 plug-ins if PLAY950 does not appear immediately.
4. Add PLAY950 to a MIDI track and confirm its editor displays version 0.13.1.

PLAY950 accepts MIDI events and exposes eleven mono audio outputs. It has no
audio input bus and no sampling or live-monitoring functionality.

IMG loading uses a bundled Universal build of AKAI Util 4.6.7. Ableton Live,
PLAY950 and IMG extraction all run natively on Apple Silicon; Rosetta is not
required. Direct P9 loading does not invoke the helper.

Install EDIT950 in `/Applications` to use **Open in EDIT950**. PLAY950 can
load and play content without EDIT950; only that convenience button
depends on the application.

PLAY950 embeds loaded programs and samples in the host project. After saving an
Ableton Set, playback recall does not require the original IMG/P9/S9 files.

AKAI Util copyright and redistribution terms are included inside the plug-in at
`Contents/Resources/AKAI-Util-NOTICE.txt`.
The corresponding source and PLAY950 compatibility change are included beside
the plug-in in `AKAI-Util-Source`.
