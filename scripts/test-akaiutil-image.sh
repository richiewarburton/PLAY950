#!/bin/sh
set -eu

if [ "$#" -ne 3 ]; then
    echo "usage: $0 <source.img> <fixture-directory> <akaiutil>" >&2
    exit 2
fi

SCRIPT_DIRECTORY=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
SOURCE_IMAGE=$1
FIXTURE_DIRECTORY=$2
AKAIUTIL=$3
OUTPUT_DIRECTORY=$(mktemp -d /tmp/play950-akai-test.XXXXXX)
cleanup() {
    rm -rf "$OUTPUT_DIRECTORY"
}
trap cleanup EXIT HUP INT TERM

"$SCRIPT_DIRECTORY/extract-akai-image.sh" \
    "$SOURCE_IMAGE" "$OUTPUT_DIRECTORY" "$AKAIUTIL"

EXTRACTED_COUNT=0
for EXTRACTED in "$OUTPUT_DIRECTORY"/*.P9 "$OUTPUT_DIRECTORY"/*.S9; do
    if [ ! -f "$EXTRACTED" ]; then
        continue
    fi
    EXTRACTED_COUNT=$((EXTRACTED_COUNT + 1))
    EXPECTED="$FIXTURE_DIRECTORY/$(basename "$EXTRACTED")"
    if [ ! -f "$EXPECTED" ]; then
        echo "no loose fixture for extracted file: $(basename "$EXTRACTED")" >&2
        exit 1
    fi
    cmp "$EXPECTED" "$EXTRACTED"
done

if [ "$EXTRACTED_COUNT" -ne 15 ]; then
    echo "file-count mismatch: expected 15, extracted $EXTRACTED_COUNT" >&2
    exit 1
fi

echo "PLAY950 AKAI Util IMG extraction passed ($EXTRACTED_COUNT byte-exact files)"
