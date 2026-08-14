#!/bin/sh
set -eu

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "usage: $0 <configured-build-directory> [output-directory]" >&2
    exit 2
fi

PLAY950_BUILD=$1
PLAY950_OUTPUT=${2:-dist}
PLAY950_BUNDLE="$PLAY950_BUILD/VST3/Release/PLAY950.vst3"
PLAY950_VALIDATOR="$PLAY950_BUILD/bin/Release/validator"

if ! xcodebuild -version >/dev/null 2>&1; then
    PLAY950_CACHED_XCODE=$(sed -n 's/^XCODE_VERSION:[^=]*=//p' \
        "$PLAY950_BUILD/CMakeCache.txt" | head -1)
    if [ -z "$PLAY950_CACHED_XCODE" ]; then
        echo "Command Line Tools build requires cached XCODE_VERSION" >&2
        exit 1
    fi
    export XCODE_VERSION=$PLAY950_CACHED_XCODE
fi

cmake -S . -B "$PLAY950_BUILD"
cmake --build "$PLAY950_BUILD" --config Release

if [ ! -d "$PLAY950_BUNDLE" ]; then
    echo "VST3 bundle not found: $PLAY950_BUNDLE" >&2
    exit 1
fi
if [ ! -x "$PLAY950_VALIDATOR" ]; then
    echo "Steinberg validator not found: $PLAY950_VALIDATOR" >&2
    exit 1
fi
if [ ! -x "$PLAY950_BUNDLE/Contents/Resources/akaiutil" ]; then
    echo "release bundle does not contain executable AKAI Util" >&2
    exit 1
fi
if [ ! -f "$PLAY950_BUNDLE/Contents/Resources/AKAI-Util-NOTICE.txt" ]; then
    echo "release bundle does not contain the AKAI Util notice" >&2
    exit 1
fi
if [ ! -f "$PLAY950_BUNDLE/Contents/Resources/AKAI-Util-GPL-2.0.txt" ]; then
    echo "release bundle does not contain the AKAI Util GPL text" >&2
    exit 1
fi
PLAY950_HELPER_ARCHS=$(lipo -archs "$PLAY950_BUNDLE/Contents/Resources/akaiutil")
case " $PLAY950_HELPER_ARCHS " in
    *" arm64 "*) ;;
    *) echo "bundled AKAI Util is missing arm64" >&2; exit 1 ;;
esac
case " $PLAY950_HELPER_ARCHS " in
    *" x86_64 "*) ;;
    *) echo "bundled AKAI Util is missing x86_64" >&2; exit 1 ;;
esac
if strings "$PLAY950_BUNDLE/Contents/MacOS/PLAY950" | grep -Fq '/Users/'; then
    echo "release binary contains a developer-machine user path" >&2
    exit 1
fi

PLAY950_ARCHS=$(lipo -archs "$PLAY950_BUNDLE/Contents/MacOS/PLAY950")
case " $PLAY950_ARCHS " in
    *" arm64 "*) ;;
    *) echo "PLAY950 binary is missing arm64" >&2; exit 1 ;;
esac
case " $PLAY950_ARCHS " in
    *" x86_64 "*) ;;
    *) echo "PLAY950 binary is missing x86_64" >&2; exit 1 ;;
esac

ctest --test-dir "$PLAY950_BUILD" -C Release --output-on-failure
"$PLAY950_VALIDATOR" "$PLAY950_BUNDLE"
codesign --verify --deep --strict "$PLAY950_BUNDLE"

PLAY950_VERSION=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' \
    "$PLAY950_BUNDLE/Contents/Info.plist")
PLAY950_MODULE_VERSION=$(plutil -extract Version raw \
    "$PLAY950_BUNDLE/Contents/Resources/moduleinfo.json")
PLAY950_PROCESSOR_VERSION=$(plutil -extract Classes.0.Version raw \
    "$PLAY950_BUNDLE/Contents/Resources/moduleinfo.json")
if [ "$PLAY950_MODULE_VERSION" != "$PLAY950_VERSION" ] || \
   [ "$PLAY950_PROCESSOR_VERSION" != "$PLAY950_VERSION" ]; then
    echo "VST3 metadata version does not match bundle version $PLAY950_VERSION" >&2
    exit 1
fi
if ! strings "$PLAY950_BUNDLE/Contents/MacOS/PLAY950" | \
        grep -Fxq "$PLAY950_VERSION"; then
    echo "PLAY950 binary does not contain bundle version $PLAY950_VERSION" >&2
    exit 1
fi
PLAY950_NAME="PLAY950-$PLAY950_VERSION-macOS-universal"
PLAY950_STAGE=$(mktemp -d /tmp/play950-release.XXXXXX)
cleanup() {
    rm -rf "$PLAY950_STAGE"
}
trap cleanup EXIT HUP INT TERM

mkdir -p "$PLAY950_OUTPUT"
mkdir -p "$PLAY950_STAGE/$PLAY950_NAME"
ditto "$PLAY950_BUNDLE" "$PLAY950_STAGE/$PLAY950_NAME/PLAY950.vst3"
cp docs/INSTALL.md "$PLAY950_STAGE/$PLAY950_NAME/README.txt"
cp LICENSE "$PLAY950_STAGE/$PLAY950_NAME/LICENSE"
cp LICENSING.md "$PLAY950_STAGE/$PLAY950_NAME/LICENSING.md"
mkdir -p "$PLAY950_STAGE/$PLAY950_NAME/Documentation/images"
cp docs/USER_GUIDE.md \
    "$PLAY950_STAGE/$PLAY950_NAME/Documentation/USER_GUIDE.md"
ditto docs/images/user-guide \
    "$PLAY950_STAGE/$PLAY950_NAME/Documentation/images/user-guide"
cp -R external/akaiutil "$PLAY950_STAGE/$PLAY950_NAME/AKAI-Util-Source"
ditto -c -k --norsrc --keepParent "$PLAY950_STAGE/$PLAY950_NAME" \
    "$PLAY950_OUTPUT/$PLAY950_NAME.zip"
(
    cd "$PLAY950_OUTPUT"
    shasum -a 256 "$PLAY950_NAME.zip" > "$PLAY950_NAME.zip.sha256"
)

echo "Created $PLAY950_OUTPUT/$PLAY950_NAME.zip"
echo "Created $PLAY950_OUTPUT/$PLAY950_NAME.zip.sha256"
