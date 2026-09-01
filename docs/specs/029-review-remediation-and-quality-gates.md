# Review remediation and quality gates

## Intent

Turn the confirmed project-review findings into deterministic behavior and make the repository's
official Qt and LLVM checks part of both local and hosted CI.

## Observable acceptance criteria

- The selected System-page device remains live: information and metric changes update the visible
  panels without changing selection or focus.
- Model updates identify their changed roles. Mandatory service, model, and telemetry dependencies
  cannot produce release-only null dereferences, and destroyed QObject dependencies are handled
  without stale-pointer access.
- Collector exceptions become service diagnostics and an error state rather than escaping Qt event
  delivery. Exclusively owned weather providers must not already have a QObject parent.
- GitHub and weather HTTP response bodies are bounded. Malformed successful GitHub payloads retain
  stale data and fail the refresh instead of publishing default values.
- Weather cache data is versioned and fully validated before publication. Invalid or legacy cache
  files are ignored atomically.
- Production QML passes `qmllint` with zero warnings. Pinned QML formatting, application
  clang-tidy, QML lint, application and daemon builds and tests, and sanitizers remain reproducible
  through the locked CI image, including clean static analysis of application object teardown.
  Local `task check` additionally enforces C++ formatting.
- Hosted CI exposes a separate required `quality` check, and releases remain gated on a successful
  CI workflow for their exact commit.

## Non-goals

- Changing telemetry wire formats, adding network providers, or changing dashboard layout.
- Vendoring workstation-local Codex review scanners. Their semantic findings are represented by
  focused tests; official Qt and LLVM tools define the enforceable baseline.
- Accepting a warning baseline or linting only changed files.

## Verification

Run focused regression tests first, then `task test`, `task check`, sanitizer presets, `task ci`,
and `git diff --check`. `all_qmllint` and the application clang-tidy build must report no
diagnostics.
