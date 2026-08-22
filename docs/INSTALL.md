# PLAY950 0.13.4 installation

PLAY950 requires macOS 14 or later and a VST3 host such as Ableton Live 12.

1. Quit Ableton Live.
2. Copy `PLAY950.vst3` to `~/Library/Audio/Plug-Ins/VST3/`.
3. Reopen Live and rescan VST3 plug-ins if PLAY950 does not appear immediately.
4. Add PLAY950 to a MIDI track and confirm its editor displays version 0.13.4.

PLAY950 accepts MIDI events and exposes eleven mono audio outputs. It has no
audio input bus and no sampling or live-monitoring functionality.

AKAI Util 4.6.7 is included inside PLAY950. No separate download, EDIT950
installation, path selection, `chmod` command or other Terminal setup is
required. PLAY950 uses the Universal helper read-only only when loading an IMG.
Ableton Live, PLAY950 and IMG extraction all run natively on Apple Silicon;
Rosetta is not required. Direct P9 loading, playback and saved-project recall do
not invoke the helper.

Install EDIT950 in `/Applications` to use **Open in EDIT950**. PLAY950 can
load and play content without EDIT950; only that convenience button
depends on the application.

PLAY950 embeds loaded programs and samples in the host project. After saving an
Ableton Set, playback recall does not require the original IMG/P9/S9 files.

PLAY950 checks GitHub for the latest release when its editor opens. If a newer
release exists, a compact button in the System panel opens the GitHub release
page. PLAY950 never downloads or installs an update automatically; quit the DAW
before replacing the VST3 manually.

AKAI Util copyright and redistribution terms are included inside the plug-in at
`Contents/Resources/AKAI-Util-NOTICE.txt`.
The corresponding source and PLAY950 compatibility change are included beside
the plug-in in `AKAI-Util-Source`.
