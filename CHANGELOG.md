# Changelog

All notable changes to Lumen are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and this project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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

[2.0.1]: https://github.com/yib7/Lumen/releases/tag/V2.0.1
[2.0.0]: https://github.com/yib7/Lumen/releases/tag/V2.0
