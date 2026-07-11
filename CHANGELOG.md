# Changelog

All notable changes to Lumen are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and this project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.3] - 2026-07-11

Robustness fixes from a second code audit, plus three backward-compatible,
opt-in features. Default rendering output is unchanged: every existing scene
renders byte-for-byte identically.

### Added
- `--gamma` flag to gamma-encode the output (sRGB-ish, 1/2.2). Off by default,
  since the bundled scenes are tuned for linear output.
- Optional camera look-at: `camera px py pz fov [lx ly lz]` aims the camera at a
  world-space point. Omitting the target keeps the default +z view, so older
  scenes are unaffected. `scenes/lookat.scene` demonstrates it.
- A live `row N/total` progress line on stderr for long renders, shown only when
  the terminal is interactive.
- The regression suite (`tests/validate.sh`) now runs in CI, with new coverage
  for non-finite input, reflect-suffix ranges, over-long lines, the light limit,
  command-line caps, and sphere/plane intersection.

### Fixed
- PNG output (the default format) no longer reports success after a short or
  failed write. Both PNG and PPM now encode through one integrity-checked writer
  that removes a truncated file and returns failure.
- Non-finite scene numbers (`nan`, `inf`) are rejected instead of passing every
  range check and reaching undefined behavior in the color conversion.
- The scene parser closes the scene file on every error path; error returns no
  longer leak the file handle.

### Changed
- `--samples`, `--depth`, and `--threads` are capped (256, 64, 1024) so absurd
  values can no longer cause integer-overflow, stack exhaustion, or a request
  for thousands of threads. The megapixel guard is now computed in a form that
  cannot itself overflow.
- The `reflect` suffix is range-checked (reflectivity 0..1, shininess > 0), and
  scene lines longer than 511 bytes are rejected instead of silently split.
- A leading UTF-8 byte-order mark in a scene file is skipped, so a file saved by
  Windows Notepad no longer fails with a misleading error.
- Shadow rays use an any-hit query that stops at the first blocker, cutting work
  on the most numerous ray type with no change to the rendered image.
- Corrected the plane-distance description in the scene format docs to match the
  implementation.

## [2.0.2] - 2026-07-07

Security and robustness release. No public API, CLI, or rendering-behavior
change.

### Fixed
- Integer overflow in the image-buffer allocation: pixel-buffer size is now
  computed in size_t on every operand, and main.c rejects absurd dimensions via
  a 100-megapixel budget cap before allocating.
- Unchecked long-to-int conversion in CLI integer parsing: values outside
  [min, INT_MAX] and strtol overflow are now rejected with a clear message.
- Box intersection returned an inward-facing normal for rays exiting a box; the
  exit face is now selected and its normal sign flipped.

### Changed
- The scene parser rejects degenerate geometry (non-positive sphere radius,
  box min >= max, zero-length plane normal, non-positive camera fov, negative
  light intensity) with line-numbered errors.
- A short/failed PPM write removes the truncated output file instead of leaving
  a corrupt image.
- vec3_normalize guards near-zero (including subnormal) lengths with a 1e-10
  threshold instead of an exact zero test.

### Added
- tests/unit_geometry.c (box exit-normal regression) and tests/validate.sh
  (CLI dimension bounds, degenerate-scene rejection, mirrors full render).

## [2.0.1] - 2026-06-29

Documentation-only release. No code or rendering behavior changed.

### Changed
- The architecture pipeline diagram in `docs/ARCHITECTURE.md` now renders as a
  mermaid flowchart on GitHub instead of ASCII art.
- The three sample-render thumbnails in the README carry descriptive alt text
  instead of one-word labels.

## [2.0.0] - 2026-06-27

The rewrite of the original Raytraced Sphere prototype into Lumen: a full
Whitted-style renderer with a scene-file format, multiple primitives and
materials, PNG output, and OpenMP parallelism. First public release under the
Lumen name.

### Added
- Whitted-style recursive raytracer in C11 with a configurable reflection
  bounce limit.
- Three primitives: spheres, infinite planes (with optional checkerboard), and
  axis-aligned boxes.
- Two materials: matte (ambient + diffuse + Phong specular) and reflective.
- Hard shadows from multiple colored point lights with distance attenuation.
- Anti-aliasing by NxN supersampling and a vertical sky gradient background.
- Plain-text scene file format with line-numbered parse errors.
- PNG output (via vendored `stb_image_write`) and PPM output, chosen by file
  extension.
- OpenMP row-parallel rendering with selectable thread count; serial fallback
  when built without OpenMP.
- Command-line interface for scene, output, resolution, samples, depth, and
  threads.
- `make`, `build.sh`, and a documented raw `gcc` invocation as build paths.
- Smoke test (`tests/smoke.sh`) and a GitHub Actions CI workflow that builds
  with `-Werror` and renders every bundled scene.

## Earlier releases

V1.0 and V1.1 (early 2025) were the original Raytraced Sphere versions, a
simpler raytracer that the 2.0 rewrite supersedes.

[2.0.3]: https://github.com/yib7/Lumen/releases/tag/v2.0.3
[2.0.2]: https://github.com/yib7/Lumen/releases/tag/v2.0.2
[2.0.1]: https://github.com/yib7/Lumen/releases/tag/v2.0.1
[2.0.0]: https://github.com/yib7/Lumen/releases/tag/v2.0.0
