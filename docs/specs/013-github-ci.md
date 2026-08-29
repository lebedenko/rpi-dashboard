# GitHub continuous integration

## Intent

Validate every proposed and integrated change with the digest-locked project CI image using the
repository's existing CMake configurations. Pull requests and pushes to `main` must exercise
the normal debug build, optimized Release build, clang-tidy, AddressSanitizer, and
UndefinedBehaviorSanitizer independently.

## Triggers and required checks

- Run for every pull request, every push to `main`, and an explicit manual dispatch.
- Execute `dev`, `release`, `tidy`, `asan`, and `ubsan` as separate matrix jobs. One configuration failing must
  not prevent the other configurations from completing, and any failed configuration fails the
  workflow.
- Cancel an older in-progress run when a newer run starts for the same workflow and Git ref.
- Grant the workflow read-only access to repository contents.

## Observable acceptance criteria

- All five configurations use Ubuntu 24.04 only as a Docker host and run in the immutable image
  locked by `ci/image.env`, with a 30-minute timeout.
- The normal, sanitizer, and clang-tidy builds compile with GCC 13, including test code that
  supplies explicit fallback values to `std::optional<QString>::value_or()`.
- The clang-tidy configuration is accepted by the clang-tidy versions provided by Ubuntu 24.04,
  while the tidy build removes GCC's `-mno-direct-extern-access` compiler argument before invoking
  clang-tidy, excludes CMake- and Qt-generated translation units under the binary directory, and
  forwards every other argument for project-owned translation units unchanged. Project code must
  also pass the shared checks with the runner-provided clang-tidy version without diagnostics caused
  by known analyzer limitations in Qt's callable and guarded-pointer implementations.
- CI receives Qt 6.8.3 with SVG support and all compilers and build tools from the project image.
- Third-party actions are pinned to immutable commit SHAs with their release versions documented
  alongside the pins.
- Each configuration invokes `scripts/run-ci-container.sh` for its matching preset; the runner then
  invokes `cmake --preset`, `cmake --build --preset`, and `ctest --preset` inside the image.
- The native daemon check configures, builds, and tests the standalone daemon with CMake and CTest
  available on the Ubuntu runner; it does not depend on optional developer task-runner tooling.
- The workflow status is visible from the repository README.
- A failure in configure, build, tests, sanitizer diagnostics, or clang-tidy produces a failed
  matrix job and therefore a failed overall workflow.

## Non-goals

- Deployment, packaging, releases, or branch-protection behavior inside this CI workflow.
- Cross-compilation, hosted ARM64 coverage, or physical Raspberry Pi display and input validation.
- Caching build directories or using the host toolchain for compilation or tests.
- Changing project dependencies.
- Removing `-mno-direct-extern-access` from normal GCC compilation or from clangd's compile flags.

## Verification

- Validate the workflows as YAML and check the patch with `git diff --check`.
- Run the focused `ci_container_runner_test` and `ci_workflow_test`, then `task test` and `task check`
  natively.
- After publishing and locking the first image, run `task ci` locally.
- After the workflow is pushed to GitHub, confirm that all four matrix jobs execute and that a
  failing matrix job makes the overall workflow fail.
