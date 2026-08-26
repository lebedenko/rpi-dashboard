# Reproducible containerized CI toolchain

## Intent

Make hosted CI and the local pre-push check use the same small, project-owned Linux toolchain.
The toolchain is published as a public OCI image and normal CI refers to it only by digest.

## Observable acceptance criteria

- The image is based on digest-pinned Ubuntu 24.04 for `linux/amd64` and reports GCC/G++ 13.3.0,
  clang-tidy 18.1.3, CMake 3.31.6, Ninja 1.13.2, and Qt 6.8.3.
- The image contains the Qt base desktop archive (including QML/Quick and SVG), build tools, and
  headless test runtime dependencies needed by this repository, but no checkout, credentials, or
  build output.
- `ci/image.env` is the sole normal-CI image lock and accepts only a GHCR repository reference with
  a SHA-256 digest. A publication-only `CI_IMAGE_OVERRIDE` may select a newly built local image.
- `scripts/run-ci-container.sh PRESET` accepts only `dev`, `tidy`, `asan`, or `ubsan`; mounts the
  checkout read-only at `/workspace`; overlays an ephemeral writable `/workspace/build`; runs the
  matching configure, build, and test presets; and returns the first failed command's status.
- Local runs use the caller's numeric user and group IDs and forward sanitizer configuration from
  the preset environment without depending on host compilers or Qt.
- `task ci` runs all four presets sequentially. Native `task test`, `task check`, and sanitizer
  tasks retain their existing behavior.
- Hosted CI uses the same runner for four independent matrix jobs and prints the locked image and
  contained tool versions.
- A manual publication workflow builds and verifies an amd64 image, runs all four presets before
  pushing it, and reports the pushed digest for a separately reviewed lock update.
- Shell tests deterministically cover lock parsing, missing Docker, invalid presets, image override,
  and container command failure propagation.

## Non-goals

- Reproducing the complete GitHub-hosted runner VM.
- ARM or Raspberry Pi validation, deployment, application behavior, or runtime API changes.
- Persisting container build directories or replacing fast native development commands.
- Automatically changing the reviewed digest lock during image publication.

## Bootstrap and upgrades

The initial `ci/image.env` deliberately records `UNPUBLISHED`: no immutable digest exists until the
first successful publication. Run the manual **Publish CI image** workflow, make the resulting GHCR
package public in its package settings, then replace the sentinel with the reported full
`ghcr.io/lebedenko/rpi-dashboard-ci@sha256:...` reference in a separate review. Later toolchain
upgrades follow the same publish-then-lock sequence; mutable tags are never consumed by normal CI.
