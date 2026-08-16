#!/bin/sh
set -eu

if [ "$#" -ne 5 ]; then
    echo "usage: $0 <source.img> <universal-akaiutil> <reference-akaiutil> <loader-test> <program-count>" >&2
    exit 2
fi

PLAY950_IMAGE=$1
PLAY950_UNIVERSAL=$2
PLAY950_REFERENCE=$3
PLAY950_LOADER=$4
PLAY950_PROGRAMS=$5
PLAY950_TEST_ROOT=$(mktemp -d /tmp/play950-universal-akaiutil.XXXXXX)
cleanup() {
    rm -rf "$PLAY950_TEST_ROOT"
}
trap cleanup EXIT HUP INT TERM

extract_with_arch() {
    PLAY950_ARCH=$1
    PLAY950_DESTINATION=$2
    PLAY950_RAW="$PLAY950_DESTINATION.raw"
    mkdir -p "$PLAY950_RAW"
    mkdir -p "$PLAY950_DESTINATION"
    printf 'lcd %s\ngetall\nq\n' "$PLAY950_RAW" |
        arch -"$PLAY950_ARCH" "$PLAY950_UNIVERSAL" -r "$PLAY950_IMAGE" >/dev/null
    for PLAY950_FILE in "$PLAY950_RAW"/*.P9 "$PLAY950_RAW"/*.S9; do
        if [ -f "$PLAY950_FILE" ]; then
            cp -p "$PLAY950_FILE" "$PLAY950_DESTINATION/"
        fi
    done
}

extract_with_arch arm64 "$PLAY950_TEST_ROOT/arm64"
extract_with_arch x86_64 "$PLAY950_TEST_ROOT/x86_64"
"$(dirname "$0")/extract-akai-image.sh" "$PLAY950_IMAGE" \
    "$PLAY950_TEST_ROOT/reference" "$PLAY950_REFERENCE"

diff -qr "$PLAY950_TEST_ROOT/arm64" "$PLAY950_TEST_ROOT/x86_64"
diff -qr "$PLAY950_TEST_ROOT/arm64" "$PLAY950_TEST_ROOT/reference"
"$PLAY950_LOADER" --program-directory "$PLAY950_TEST_ROOT/arm64" "$PLAY950_PROGRAMS"
"$PLAY950_LOADER" --program-directory "$PLAY950_TEST_ROOT/x86_64" "$PLAY950_PROGRAMS"
