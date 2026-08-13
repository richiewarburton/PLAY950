#!/bin/sh
set -eu

if [ "$#" -ne 3 ]; then
    echo "usage: $0 <source.img> <output-directory> <akaiutil>" >&2
    exit 2
fi

SOURCE_IMAGE=$1
OUTPUT_DIRECTORY=$2
AKAIUTIL=$3

if [ ! -f "$SOURCE_IMAGE" ]; then
    echo "source image does not exist: $SOURCE_IMAGE" >&2
    exit 1
fi
if [ ! -x "$AKAIUTIL" ]; then
    echo "AKAI Util is not executable: $AKAIUTIL" >&2
    exit 1
fi
if [ -e "$OUTPUT_DIRECTORY" ] && find "$OUTPUT_DIRECTORY" -mindepth 1 -print -quit | grep -q .; then
    echo "output directory must be empty: $OUTPUT_DIRECTORY" >&2
    exit 1
fi

STAGING_DIRECTORY=$(mktemp -d /tmp/play950-akai-image.XXXXXX)
cleanup() {
    rm -rf "$STAGING_DIRECTORY"
}
trap cleanup EXIT HUP INT TERM

LOG_FILE="$STAGING_DIRECTORY/akaiutil.log"
printf 'lcd %s\ngetall\nq\n' "$STAGING_DIRECTORY" |
    "$AKAIUTIL" -r "$SOURCE_IMAGE" >"$LOG_FILE" 2>&1

set -- "$STAGING_DIRECTORY"/*.P9 "$STAGING_DIRECTORY"/*.S9
if [ ! -e "$1" ]; then
    echo "AKAI Util exported no S9/P9 files" >&2
    sed -n '1,160p' "$LOG_FILE" >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIRECTORY"
for EXPORTED_FILE in "$@"; do
    if [ -f "$EXPORTED_FILE" ]; then
        cp -p "$EXPORTED_FILE" "$OUTPUT_DIRECTORY/"
    fi
done
