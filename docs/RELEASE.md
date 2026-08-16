# PLAY950 macOS release procedure

## Supported package

PLAY950 0.13.3 targets macOS 14 or later and Ableton Live 12. The VST3 binary is
Universal (`arm64` and `x86_64`). IMG extraction uses the bundled Universal AKAI
Util 4.6.7 helper and does not require Rosetta. Direct P9 loading does not invoke
AKAI Util.

AKAI Util is redistributed with its original copyright and redistribution notice
at `PLAY950.vst3/Contents/Resources/AKAI-Util-NOTICE.txt`. Its corresponding GPL
source and the small Apple-SDK compatibility change ship in `AKAI-Util-Source`.

## Reproducible release build

Configure a fresh build rather than reusing the development build:

```sh
XCODE_VERSION=26.0 cmake -S . -B build-release -G "Unix Makefiles" \
  -DXCODE_VERSION=26.0 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
  -DPLAY950_BUNDLE_AKAIUTIL=ON \
  -DPLAY950_AKAIUTIL=/absolute/path/to/trusted-4.6.7-akaiutil
scripts/package-release.sh build-release
```

`XCODE_VERSION` is required only when this legacy VST3 SDK is configured against
Apple Command Line Tools rather than a full Xcode installation. Omit it on the
final signing/notarization machine, where full Xcode is required.

The trusted binary is used only for byte-for-byte regression comparison. The
packaging script rejects a non-Universal plug-in or helper, missing licence/source
materials, failed automated tests, failed Steinberg validation, or an invalid
code seal. It creates a ZIP and SHA-256 file under `dist/`.

## Developer ID signing and notarization

The local build is ad-hoc signed and suitable for development testing only. A
public release needs full Xcode, an Apple Developer account, a `Developer ID
Application` certificate and notarization credentials.

For the final candidate, sign the nested helper first and the VST3 bundle last:

```sh
codesign --force --options runtime --timestamp \
  --sign "Developer ID Application: NAME (TEAMID)" \
  PLAY950.vst3/Contents/Resources/akaiutil
codesign --force --deep --strict --options runtime --timestamp \
  --sign "Developer ID Application: NAME (TEAMID)" PLAY950.vst3
codesign --verify --deep --strict --verbose=2 PLAY950.vst3
```

ZIP with `ditto`, submit it with `xcrun notarytool`, wait for acceptance, then
staple and reassess it:

```sh
ditto -c -k --keepParent PLAY950.vst3 PLAY950-notarization.zip
xcrun notarytool submit PLAY950-notarization.zip --keychain-profile PLAY950 --wait
xcrun stapler staple PLAY950.vst3
spctl --assess --type install --verbose=2 PLAY950.vst3
```

## Clean-machine acceptance

Test the exact ZIP on a Mac that has never used the development tree:

1. Run native Apple Silicon Live without Rosetta.
2. Copy `PLAY950.vst3` to `~/Library/Audio/Plug-Ins/VST3/`.
3. Reboot or force Live to rescan VST3 plug-ins.
4. Confirm version 0.13.3, confirm that the host exposes no audio input bus, and
   load a P9 with linked S9 files.
5. Load a multi-program IMG and verify selection, Reload IMG and recent images.
6. Verify the editor starts in System theme, follows macOS light/dark appearance,
   and can be held explicitly in Light or Dark from the Theme menu.
7. Save a Set, remove the source, and verify embedded recall.
8. Install EDIT950 and verify Open in EDIT950; without it, playback and
   loading must remain functional and the button must report that it is absent.
9. Repeat on native Apple Silicon Live and an Intel or Rosetta-hosted Live.

## Native development-machine smoke test

The accepted Apple-silicon smoke test loaded a multi-program IMG, switched and
played its programs, reloaded the image after an edit, opened the source in
EDIT950 and recalled the sound from a saved Ableton Set. The source was removed
before recall to confirm that the Set carried everything needed for playback.

## Clean M1 acceptance

The packaged ZIP also passed on a separate Apple-silicon Mac. Ableton Live ran
natively; the bundled helper reported `x86_64 arm64`. Native IMG loading,
multi-program discovery, Reload IMG with selection retention and saved-Set
recall all passed without Rosetta.
