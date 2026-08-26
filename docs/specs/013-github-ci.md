# GitHub continuous integration

## Intent

Validate every proposed and integrated change on a reproducible hosted Linux environment using
the repository's existing CMake configurations. Pull requests and pushes to `main` must exercise
the normal debug build, clang-tidy, AddressSanitizer, and UndefinedBehaviorSanitizer independently.

## Triggers and required checks

- Run for every pull request, every push to `main`, and an explicit manual dispatch.
- Execute `dev`, `tidy`, `asan`, and `ubsan` as separate matrix jobs. One configuration failing must
  not prevent the other configurations from completing, and any failed configuration fails the
  workflow.
- Cancel an older in-progress run when a newer run starts for the same workflow and Git ref.
- Grant the workflow read-only access to repository contents.

## Observable acceptance criteria

- All four configurations run on the pinned Ubuntu 24.04 hosted runner with a 30-minute timeout.
- CI installs Qt 6.8 with Qt SVG support from the base desktop archive and caches only the
  downloaded Qt installation. It must not request Qt SVG as a separately downloadable module.
- Third-party actions are pinned to immutable commit SHAs with their release versions documented
  alongside the pins.
- Each configuration invokes its matching repository configure, build, and test presets directly:
  `cmake --preset`, `cmake --build --preset`, and `ctest --preset`.
- The workflow status is visible from the repository README.
- A failure in configure, build, tests, sanitizer diagnostics, or clang-tidy produces a failed
  matrix job and therefore a failed overall workflow.

## Non-goals

- Deployment, packaging, releases, or branch-protection configuration.
- Cross-compilation, hosted ARM64 coverage, or physical Raspberry Pi display and input validation.
- Caching build directories or introducing an additional task-runner dependency in CI.
- Changing application APIs, CMake targets, presets, or project dependencies.

## Verification

- Validate the workflow as YAML and check the patch with `git diff --check`.
- Run `task test`, `task check`, `task test-asan`, and `task test-ubsan` locally.
- After the workflow is pushed to GitHub, confirm that all four matrix jobs execute and that a
  failing matrix job makes the overall workflow fail.
