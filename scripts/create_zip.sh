#!/bin/bash

dir="$(git rev-parse --show-toplevel)"

OUT="${1:-$dir/release.zip}"
INCLUDE_FILE="${2:-packaging/zip-include.txt}"
EXCLUDE_FILE="${3:-packaging/zip-ignore.txt}"

cmd=(zip -r "$OUT" .)

if [[ -f "$INCLUDE_FILE" && -s "$INCLUDE_FILE" ]]; then
  cmd+=(-i@"$INCLUDE_FILE")
fi

if [[ -f "$EXCLUDE_FILE" && -s "$EXCLUDE_FILE" ]]; then
  cmd+=(-x@"$EXCLUDE_FILE")
fi

"${cmd[@]}"
echo "Created $OUT"
