#!/usr/bin/env bash
# Sanitizer harness (audit finding P2-9): build Lumen under AddressSanitizer +
# UndefinedBehaviorSanitizer and exercise the scene parser -- the real attack
# surface named in SECURITY.md -- so UB like the NaN->byte cast (SP1) or a
# parser edge case aborts the run instead of silently corrupting output.
#
# The sanitizer runtimes are not always available: the local WinLibs MinGW
# toolchain ships no libasan/libubsan, so `-fsanitize` fails to link there.
# We probe for them first and SKIP cleanly (exit 0) when absent; the Ubuntu CI
# gcc has them, so the suite runs for real in CI.
#
# Cases exercised (single-threaded -- no -fopenmp -- for deterministic ASan
# output):
#   * all bundled scenes and a coincident-light scene must render with exit 0
#     and NO sanitizer diagnostic;
#   * malformed scenes (non-finite value, huge finite value, extra tokens) may
#     be rejected (non-zero exit) but must also show NO sanitizer diagnostic.
# Exits non-zero on any sanitizer diagnostic or unexpected render failure.
set -uo pipefail

cd "$(dirname "$0")/.."

CC=${CC:-gcc}
BIN=lumen_asan
TMP=$(mktemp -d)
trap 'rm -f "$BIN" "$BIN.exe"; rm -rf "$TMP"' EXIT

fail=0
pass() { echo "  ok: $1"; }
bad()  { echo "  FAIL: $1"; fail=1; }

# Substrings that only appear when a sanitizer actually fires. A clean rejection
# by the parser (exit non-zero with a "line N: ..." message) matches none of
# these; a real ASan/UBSan abort always prints at least one.
SANI_RE='runtime error|AddressSanitizer|UndefinedBehaviorSanitizer|SUMMARY:|ERROR: libFuzzer'

# --- Availability probe -----------------------------------------------------
# Compile+link a trivial program with both sanitizers. If this fails, the
# runtimes are missing (the local MinGW path): skip, do not fail.
printf 'int main(void){return 0;}\n' > "$TMP/probe.c"
if ! "$CC" -fsanitize=address,undefined -O1 -std=c11 "$TMP/probe.c" \
        -o "$TMP/probe" >/dev/null 2>&1; then
  echo "sanitizers unavailable with $CC (no libasan/libubsan) - skipping"
  echo "SANITIZER SUITE SKIPPED"
  exit 0
fi

# --- Build ------------------------------------------------------------------
echo "Building Lumen under ASan+UBSan ($CC)..."
if ! "$CC" -fsanitize=address,undefined -g -O1 -std=c11 -Isrc \
        src/*.c -lm -o "$BIN"; then
  echo "SANITIZER SUITE FAILED"; exit 1
fi

# Any diagnostic must abort the process and print a stack trace, so a nonzero
# exit paired with sanitizer output is unambiguous.
export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
export ASAN_OPTIONS=halt_on_error=1

# Run the binary on a scene file, capturing stderr. Sets globals:
#   RC  -- process exit code
#   ERR -- captured stderr text
run_scene() { # scene-path [extra args...]
  local scene="$1"; shift
  ERR=$(./"$BIN" --scene "$scene" --output "$TMP/out.ppm" \
        --width 64 --height 48 --samples 2 "$@" 2>&1 >/dev/null)
  RC=$?
}

# A case that must render successfully AND be sanitizer-clean.
expect_clean_render() { # label scene-path [extra args...]
  local label="$1"; shift
  run_scene "$@"
  if printf '%s' "$ERR" | grep -Eq "$SANI_RE"; then
    bad "$label: sanitizer diagnostic ($ERR)"
  elif [ "$RC" -ne 0 ]; then
    bad "$label: render failed rc=$RC ($ERR)"
  else
    pass "$label rendered clean"
  fi
}

# A malformed case: may exit non-zero, but must NOT trip a sanitizer.
expect_no_sanitizer() { # label scene-path
  local label="$1"; shift
  run_scene "$@"
  if printf '%s' "$ERR" | grep -Eq "$SANI_RE"; then
    bad "$label: sanitizer diagnostic on malformed input ($ERR)"
  else
    pass "$label rejected/handled without sanitizer error (rc=$RC)"
  fi
}

echo "== bundled scenes render clean under sanitizers =="
for scene in scenes/*.scene; do
  expect_clean_render "$(basename "$scene")" "$scene"
done

echo "== coincident-light scene renders clean =="
# Light placed exactly on the sphere apex (0,0,4): to_light is zero-length, so a
# naive normalize yields NaN -- UBSan catches the NaN->unsigned char cast if the
# SP1 guard regresses. A 3x3 image at samples=1 sends the center ray straight
# down (0,0,1) onto the apex.
printf '%b' 'camera 0 0 0 0.5\nsphere 0 0 5 1 1 0 0\nlight 0 0 4 1 1 1 1\n' \
  > "$TMP/coincident.scene"
ERR=$(./"$BIN" --scene "$TMP/coincident.scene" --output "$TMP/out.ppm" \
      --width 3 --height 3 --samples 1 2>&1 >/dev/null)
RC=$?
if printf '%s' "$ERR" | grep -Eq "$SANI_RE"; then
  bad "coincident-light: sanitizer diagnostic ($ERR)"
elif [ "$RC" -ne 0 ]; then
  bad "coincident-light: render failed rc=$RC ($ERR)"
else
  pass "coincident-light rendered clean"
fi

echo "== malformed scenes are handled without sanitizer aborts =="
# Non-finite value: must be rejected by the finite check, not fed to a cast.
printf '%b' 'camera 0 0 0 0.5\nsphere 0 0 5 nan 1 0 0\n' > "$TMP/nan.scene"
expect_no_sanitizer "non-finite value"     "$TMP/nan.scene"
# Huge but finite value: exercises arithmetic without overflowing to inf.
printf '%b' 'camera 0 0 0 0.5\nsphere 0 0 5e300 1 1 0 0\n' > "$TMP/huge.scene"
expect_no_sanitizer "huge finite value"    "$TMP/huge.scene"
# Too many tokens on one line: parser must reject, not read past its array.
printf '%b' 'camera 0 0 0 0.5\nsphere 0 0 5 1 1 0 0 9 9 9\n' > "$TMP/extra.scene"
expect_no_sanitizer "extra tokens"         "$TMP/extra.scene"

if [ "$fail" -ne 0 ]; then
  echo "SANITIZER SUITE FAILED"; exit 1
fi
echo "SANITIZER SUITE PASSED"
