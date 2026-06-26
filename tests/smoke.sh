#!/usr/bin/env bash
# Smoke test: build Lumen, render every bundled scene, and confirm each output
# is a valid PNG of the expected size. Exits non-zero on any failure.
set -euo pipefail

cd "$(dirname "$0")/.."

CC=${CC:-gcc}
echo "Building with $CC..."
$CC -O2 -fopenmp -std=c11 -Wall -Wextra -Isrc src/*.c -lm -o lumen_test

fail=0
for scene in scenes/*.scene; do
  name=$(basename "$scene" .scene)
  out="smoke_${name}.png"
  echo "Rendering $scene -> $out"
  ./lumen_test --scene "$scene" --output "$out" --width 200 --height 150 --samples 1 >/dev/null

  if [ ! -s "$out" ]; then
    echo "FAIL: $out was not produced"; fail=1; continue
  fi
  # PNG magic number is the 8 bytes 89 50 4E 47 0D 0A 1A 0A
  magic=$(head -c 8 "$out" | od -An -tx1 | tr -d ' \n')
  if [ "$magic" != "89504e470d0a1a0a" ]; then
    echo "FAIL: $out is not a PNG (magic=$magic)"; fail=1; continue
  fi
  echo "  ok ($(wc -c < "$out") bytes)"
  rm -f "$out"
done

rm -f lumen_test lumen_test.exe
if [ "$fail" -ne 0 ]; then
  echo "SMOKE TEST FAILED"; exit 1
fi
echo "SMOKE TEST PASSED"
