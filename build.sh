#!/usr/bin/env bash
# Build script for Lumen. Compiles all sources in src/ with OpenMP and -O2.
# Usage: ./build.sh            (release build -> lumen)
#        ./build.sh debug      (debug build with -g -O0 and no OpenMP)
set -euo pipefail

OUT="lumen"
CC="${CC:-gcc}"
COMMON="-std=c11 -Wall -Wextra -Isrc -lm"

if [ "${1:-}" = "debug" ]; then
    echo "building debug -> ${OUT}"
    $CC -g -O0 $COMMON src/*.c -o "$OUT"
else
    echo "building release -> ${OUT}"
    $CC -O2 -fopenmp $COMMON src/*.c -o "$OUT"
fi

echo "done: ${OUT}"
