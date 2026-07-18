#!/usr/bin/env bash
# Regression tests for the 2026-07 security/robustness audit fixes (Audit 3 + 5):
#   - command-line dimension bounds (integer-overflow guard)
#   - command-line value caps (--samples/--depth/--threads upper bounds)
#   - degenerate scene rejection with line-numbered errors, including non-finite
#     (nan/inf) numbers, out-of-range reflect suffixes, over-long lines, and the
#     max-lights limit
#   - sphere/plane/box intersection unit checks
#   - an exact-size sweep over a rendered PPM (mirrors scene)
#   - feature coverage: --gamma changes the encoded output, and the look-at
#     camera form renders a complete image
# Exits non-zero on any failure.
set -uo pipefail

cd "$(dirname "$0")/.."

CC=${CC:-gcc}
BIN=validate_lumen
UNIT=validate_unit
TMP=$(mktemp -d)
trap 'rm -f "$BIN" "$BIN.exe" "$UNIT" "$UNIT.exe"; rm -rf "$TMP"' EXIT

fail=0
pass() { echo "  ok: $1"; }
bad()  { echo "  FAIL: $1"; fail=1; }

echo "Building Lumen ($CC)..."
$CC -O2 -std=c11 -Wall -Wextra -Werror -Isrc src/*.c -lm -o "$BIN"
echo "Building geometry unit test..."
$CC -std=c11 -Wall -Wextra -Werror -Isrc tests/unit_geometry.c \
    src/geometry.c src/vec3.c -lm -o "$UNIT"

echo "== box exit-normal unit checks =="
if ./"$UNIT" >/dev/null; then pass "unit_geometry"; else bad "unit_geometry"; fi

echo "== command-line dimension bounds =="
# Overflow-sized dimensions must be rejected, not wrapped into a small buffer.
if ! ./"$BIN" --width 50000 --height 50000 --output "$TMP/o.png" >/dev/null 2>&1; then
  # Rejected either by explicit bound or by a failed (but checked) allocation.
  pass "huge dimensions rejected"
else
  bad "huge dimensions accepted"
fi
if ! ./"$BIN" --width 2147483648 --output "$TMP/o.png" >/dev/null 2>&1; then
  pass "INT_MAX+1 width rejected"; else bad "INT_MAX+1 width accepted"; fi
if ! ./"$BIN" --width -5 --output "$TMP/o.png" >/dev/null 2>&1; then
  pass "negative width rejected"; else bad "negative width accepted"; fi
# --threads 0 (auto) must still be accepted.
if ./"$BIN" --scene scenes/solar.scene --width 8 --height 6 --samples 1 \
     --threads 0 --output "$TMP/t0.png" >/dev/null 2>&1; then
  pass "--threads 0 (auto) accepted"; else bad "--threads 0 rejected"; fi

echo "== command-line value caps (samples/depth/threads) =="
# Absurd-but-parseable values must be rejected up front, not fed to n*n / deep
# recursion / thousands of real threads.
for cap in "--samples 100000" "--depth 100000" "--threads 9999999"; do
  if ! ./"$BIN" $cap --scene scenes/solar.scene --width 8 --height 6 \
       --output "$TMP/o.png" >/dev/null 2>&1; then
    pass "over-cap rejected: $cap"
  else
    bad "over-cap accepted: $cap"
  fi
done
# The documented ceilings themselves must still be accepted.
if ./"$BIN" --scene scenes/solar.scene --width 8 --height 6 \
     --samples 256 --depth 64 --threads 1024 --output "$TMP/o.png" >/dev/null 2>&1; then
  pass "boundary samples=256 depth=64 threads=1024 accepted"
else
  bad "boundary samples/depth/threads rejected"
fi

echo "== bare -h prints usage; -h <N> still sets height =="
# A bare -h with no following value is a help request (the near-universal -h
# convention): it must exit 0 and print the usage text, not error.
out=$(./"$BIN" -h 2>&1)
if [ $? -eq 0 ] && printf '%s' "$out" | grep -q "Usage:"; then
  pass "bare -h prints usage and exits 0"
else
  bad "bare -h did not print usage / exit 0 (got: $out)"
fi
# -h <N> must still set the height: render a tiny image at height 123.
if ./"$BIN" --scene scenes/solar.scene -w 8 -h 123 --samples 1 \
     --output "$TMP/h123.ppm" >/dev/null 2>&1 && [ -s "$TMP/h123.ppm" ]; then
  pass "-h 123 still sets height (render succeeded)"
else
  bad "-h 123 failed to set height / render"
fi

echo "== degenerate scene rejection (line-numbered) =="
reject_scene() { # name, content, expected-substring
  printf '%b' "$2" > "$TMP/$1.scene"
  out=$(./"$BIN" --scene "$TMP/$1.scene" --output "$TMP/o.png" 2>&1)
  if [ $? -ne 0 ] && printf '%s' "$out" | grep -q "$3"; then
    pass "$1 -> $out"
  else
    bad "$1 not rejected as expected (got: $out)"
  fi
}
reject_scene bad_radius 'camera 0 0 0 0.5\nsphere 0 0 5 -1 1 0 0\n'        'radius must be'
reject_scene bad_box    'camera 0 0 0 0.5\nbox 1 1 1 0 0 0 1 0 0\n'        'box min must'
reject_scene bad_plane  'camera 0 0 0 0.5\nplane 0 0 0 1 1 1 1 0 0 0\n'    'plane normal must'
reject_scene bad_fov    'camera 0 0 0 0\nsphere 0 0 5 1 1 0 0\n'           'fov must'
reject_scene bad_light  'camera 0 0 0 0.5\nlight 0 0 0 1 1 1 -2\nsphere 0 0 5 1 1 0 0\n' 'intensity must'
# Non-finite numbers (nan/inf) must be rejected, not silently pass every range
# check by comparing false against everything.
reject_scene bad_nan    'camera 0 0 0 0.5\nsphere 0 0 5 nan 1 0 0\n'       'finite'
reject_scene bad_inf    'camera 0 0 0 inf\nsphere 0 0 5 1 1 0 0\n'         'finite'
# reflect suffix out of range.
reject_scene bad_refl   'camera 0 0 0 0.5\nsphere 0 0 5 1 1 0 0 reflect 5 60\n'  'reflectivity must'
reject_scene bad_shine  'camera 0 0 0 0.5\nsphere 0 0 5 1 1 0 0 reflect 0.5 0\n' 'shininess must'
# A line longer than the 511-byte buffer must be rejected, not split in two.
long_pad=$(printf '#%.0s' $(seq 1 600))
reject_scene bad_long   "camera 0 0 0 0.5\nsphere 0 0 5 1 1 0 0 ${long_pad}\n" 'too long'
# More than MAX_LIGHTS (8) lights must be rejected on the 9th.
nine_lights='camera 0 0 0 0.5\n'
for _ in $(seq 1 9); do nine_lights="${nine_lights}light 0 5 0 1 1 1 1\n"; done
nine_lights="${nine_lights}sphere 0 0 5 1 1 0 0\n"
reject_scene bad_lights "$nine_lights" 'too many lights'
# A second camera or sky directive must be rejected, not silently overwrite the
# first.
reject_scene dup_camera 'camera 0 0 0 0.5\ncamera 0 0 0 0.5\nsphere 0 0 5 1 1 0 0\n' 'camera already defined'
reject_scene dup_sky    'camera 0 0 0 0.5\nsky 0 0 0 1 1 1\nsky 0 0 0 1 1 1\nsphere 0 0 5 1 1 0 0\n' 'sky already defined'

echo "== embedded NUL byte rejected accurately (not 'too long') =="
# A short, newline-terminated line that contains an embedded 0x00 must be
# rejected with an accurate message. The old fgets+strchr reader misdiagnosed
# this as "line too long" -- strchr stops at the NUL and never sees the real
# newline that follows -- so the reader must instead name the NUL and must NOT
# say "too long". A text scene file never legitimately contains a NUL, and the
# tokenizer cannot read past one anyway.
printf '%b' 'camera 0 0 0 0.5\nsphere 0 0\x005 1 1 0 0\n' > "$TMP/bad_nul.scene"
out=$(./"$BIN" --scene "$TMP/bad_nul.scene" --output "$TMP/o.png" 2>&1)
if [ $? -ne 0 ] \
   && printf '%s' "$out" | grep -qi 'NUL' \
   && ! printf '%s' "$out" | grep -q 'too long'; then
  pass "embedded NUL rejected accurately -> $out"
else
  bad "embedded NUL not rejected accurately (got: $out)"
fi

echo "== exactly-511-byte final line without trailing newline is accepted =="
# A final line of exactly LINE_BUF-1 = 511 bytes with no trailing newline is a
# complete, legal line. The old reader rejected it as "too long" because fgets
# filled the 511-byte buffer without reaching a newline and feof() was not yet
# set on that read. Build such a line: a valid sphere padded with a trailing
# '#'-comment (tokenize ignores everything from '#' on) to exactly 511 bytes.
line511="sphere 0 0 5 1 1 0 0 "
need=$((511 - ${#line511}))
line511="${line511}$(printf '#%.0s' $(seq 1 "$need"))"
if [ "${#line511}" -ne 511 ]; then
  bad "test bug: final line is ${#line511} bytes, expected 511"
else
  # No trailing newline: the sphere line is the file's exact-511-byte last line.
  printf 'camera 0 0 0 0.5\n%s' "$line511" > "$TMP/line511.scene"
  last_len=$(tail -n 1 "$TMP/line511.scene" | wc -c)   # bytes of the last line
  if ./"$BIN" --scene "$TMP/line511.scene" --output "$TMP/line511.ppm" \
       --width 8 --height 6 --samples 1 >/dev/null 2>&1; then
    pass "511-byte final line accepted (len=${#line511}, tail wc -c=$last_len)"
  else
    bad "511-byte final line rejected (len=${#line511}, should be accepted)"
  fi
fi

echo "== reflective-box scene renders fully (exit-normal regression) =="
# mirrors.scene exercises boxes whose reflected rays start inside and exit;
# the box exit-normal fix is unit-tested above. Here we confirm the whole
# scene still renders to a complete, correctly sized PPM.
if ./"$BIN" --scene scenes/mirrors.scene --output "$TMP/mirrors.ppm" \
     --width 120 --height 90 --samples 1 >/dev/null 2>&1; then
  bytes=$(wc -c < "$TMP/mirrors.ppm")
  # header "P6\n120 90\n255\n" (14 bytes) + 120*90*3 (32400) = 32414 bytes exactly.
  # An exact check catches a file truncated by up to the 14-byte header, which a
  # >= 32400 check would have passed.
  if [ "$bytes" -eq 32414 ]; then
    pass "mirrors.ppm rendered fully ($bytes bytes)"
  else
    bad "mirrors.ppm wrong size ($bytes bytes, expected 32414)"
  fi
else
  bad "mirrors.scene render failed"
fi

echo "== --gamma changes the output encoding =="
# --gamma applies pow(1/2.2) before quantizing, so the encoded image must differ
# from the default linear output. Render the same scene both ways and confirm the
# bytes differ, so a no-op --gamma flag can't pass silently.
./"$BIN" --scene scenes/solar.scene --output "$TMP/linear.ppm" \
     --width 60 --height 45 --samples 1 >/dev/null 2>&1
./"$BIN" --scene scenes/solar.scene --output "$TMP/gamma.ppm" \
     --width 60 --height 45 --samples 1 --gamma >/dev/null 2>&1
if [ -s "$TMP/linear.ppm" ] && [ -s "$TMP/gamma.ppm" ] \
   && ! cmp -s "$TMP/linear.ppm" "$TMP/gamma.ppm"; then
  pass "--gamma output differs from linear"
else
  bad "--gamma output identical to linear (or a render failed)"
fi

echo "== light coincident with surface renders without NaN =="
# A light placed exactly on a hit point makes to_light zero-length; naively
# normalizing it yields (0,0,0)*Inf = NaN, which slips past the diff<=0 guard
# and casts a NaN double to unsigned char (undefined behavior). With an odd
# NxN image at --samples 1 the center ray is exactly (0,0,1), so it strikes the
# sphere apex at (0,0,4) -- exactly the light's position. The fix must render a
# complete, correctly sized PPM with exit 0.
printf '%b' 'camera 0 0 0 0.5\nsphere 0 0 5 1 0.8 0.3 0.2\nlight 0 0 4 1 1 1 1\n' \
  > "$TMP/coincident.scene"
if ./"$BIN" --scene "$TMP/coincident.scene" --output "$TMP/coincident.ppm" \
     --width 3 --height 3 --samples 1 >/dev/null 2>&1; then
  bytes=$(wc -c < "$TMP/coincident.ppm")
  # header "P6\n3 3\n255\n" (11 bytes) + 3*3*3 (27) = 38 bytes exactly.
  if [ "$bytes" -eq 38 ]; then
    pass "coincident-light scene rendered fully ($bytes bytes)"
  else
    bad "coincident-light scene wrong size ($bytes bytes, expected 38)"
  fi
else
  bad "coincident-light scene render failed"
fi

echo "== camera look-at scene renders fully =="
# scenes/lookat.scene uses the optional 7-arg 'camera px py pz fov lx ly lz'
# look-at form; confirm that code path renders a complete, correctly sized image.
if ./"$BIN" --scene scenes/lookat.scene --output "$TMP/lookat.ppm" \
     --width 120 --height 90 --samples 1 >/dev/null 2>&1; then
  bytes=$(wc -c < "$TMP/lookat.ppm")
  if [ "$bytes" -eq 32414 ]; then
    pass "lookat.ppm rendered fully ($bytes bytes)"
  else
    bad "lookat.ppm wrong size ($bytes bytes, expected 32414)"
  fi
else
  bad "lookat.scene render failed"
fi

echo "== golden image: solar.scene pixel values are byte-exact (shading guard) =="
# Every other render check in this file is STRUCTURAL -- smoke.sh checks the PNG
# magic bytes, and the size checks above verify exact PPM byte counts. NONE of
# them inspect a pixel's color, so a regression in the shading/attenuation
# formula, checkerboard parity, reflection blend, or sky gradient would change
# pixel COLORS without changing the file size or header, and would slip through.
# This block pins the actual pixel bytes.
#
# solar.scene at 40x30 --samples 1 is fully deterministic: single-sample means no
# sub-pixel averaging spread, OpenMP only parallelizes across independent rows
# (per-pixel results are identical run-to-run and across thread counts), and PPM
# is raw RGB -- no PNG/zlib compression variance. The header is a fixed 13 bytes,
# "P6\n40 30\n255\n", so body pixel (x,y) starts at file offset 13 + (y*40+x)*3.
#
# Golden values below were captured from HEAD. On failure, the named-pixel lines
# say WHICH color regressed; the sha256 confirms the whole image moved.
GOLD_SHA=7cb7d176ac8897f0a50bf4f72cb0791451cd60680d2a9a371db648a26dc0c737
gsha() { # sha256 of a file, hash field only; sha256sum with an openssl fallback
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$1" | cut -d' ' -f1
  else
    openssl dgst -sha256 "$1" | awk '{print $NF}'
  fi
}
gpx() { # space-joined RGB triple of body pixel (x=$1, y=$2) in the 40-wide PPM $3
  local off=$((13 + (($2 * 40 + $1) * 3)))
  dd if="$3" bs=1 skip="$off" count=3 2>/dev/null | od -An -tu1 | tr -s ' ' | sed 's/^ *//;s/ *$//'
}
if ./"$BIN" --scene scenes/solar.scene --output "$TMP/gold.ppm" \
     --width 40 --height 30 --samples 1 >/dev/null 2>&1; then
  gbytes=$(wc -c < "$TMP/gold.ppm")
  if [ "$gbytes" -ne 3613 ]; then
    # header 13 + 40*30*3 (3600) = 3613; wrong size means the render itself broke.
    bad "golden render wrong size ($gbytes bytes, expected 3613)"
  else
    got_sha=$(gsha "$TMP/gold.ppm")
    if [ "$got_sha" = "$GOLD_SHA" ]; then
      pass "golden sha256 matches ($got_sha)"
    else
      bad "golden sha256 MISMATCH (got $got_sha) -- shading/parity/reflection/sky regressed"
    fi
    # Named spot-checks so a failure names the region, not just "hash differs".
    sky=$(gpx 0 0 "$TMP/gold.ppm")       # top-left: a sky-gradient pixel
    if [ "$sky" = "104 147 229" ]; then
      pass "golden sky pixel (0,0) = $sky"
    else
      bad "golden sky pixel (0,0) changed (got '$sky', want '104 147 229') -- sky gradient regressed"
    fi
    floor=$(gpx 20 29 "$TMP/gold.ppm")   # bottom-center: a reflective checker-floor pixel
    if [ "$floor" = "37 48 75" ]; then
      pass "golden floor pixel (20,29) = $floor"
    else
      bad "golden floor pixel (20,29) changed (got '$floor', want '37 48 75') -- checker/reflection/attenuation regressed"
    fi
    sphere=$(gpx 24 7 "$TMP/gold.ppm")   # the yellow sphere: diffuse + specular shading
    if [ "$sphere" = "181 150 23" ]; then
      pass "golden sphere pixel (24,7) = $sphere"
    else
      bad "golden sphere pixel (24,7) changed (got '$sphere', want '181 150 23') -- diffuse/specular shading regressed"
    fi
  fi
else
  bad "golden solar.scene render failed"
fi

if [ "$fail" -ne 0 ]; then
  echo "VALIDATION TESTS FAILED"; exit 1
fi
echo "VALIDATION TESTS PASSED"
