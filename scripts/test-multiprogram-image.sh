#!/bin/sh
set -eu

if [ "$#" -ne 4 ]; then
    echo "usage: $0 <source.img> <akaiutil> <loader-test> <expected-program-count>" >&2
    exit 2
fi

SCRIPT_DIRECTORY=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
SOURCE_IMAGE=$1
AKAIUTIL=$2
LOADER_TEST=$3
EXPECTED_PROGRAM_COUNT=$4
OUTPUT_DIRECTORY=$(mktemp -d /tmp/play950-multiprogram-test.XXXXXX)
cleanup() {
    rm -rf "$OUTPUT_DIRECTORY"
}
trap cleanup EXIT HUP INT TERM

"$SCRIPT_DIRECTORY/extract-akai-image.sh" \
    "$SOURCE_IMAGE" "$OUTPUT_DIRECTORY/export" "$AKAIUTIL"
"$LOADER_TEST" --program-directory "$OUTPUT_DIRECTORY/export" \
    "$EXPECTED_PROGRAM_COUNT"
