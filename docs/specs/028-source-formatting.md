# C++ and QML source formatting

## Scope

Provide one development command that formats the project's C++ and QML sources and one
non-modifying command that enforces the same formatting baseline. Formatting covers matching
source files under `src/` and `tests/`.

## Acceptance criteria

- `task format` formats `*.cpp` and `*.h` files with `clang-format` and `*.qml` files with
  `qmlformat` under `src/` and `tests/`.
- QML formatting uses the default `qmlformat` conventions, including import sorting.
- `task format:check` does not modify source files.
- `task format:check` fails when a covered C++ or QML file differs from formatter output or when a
  formatter reports an error.
- A QML formatting mismatch produces a unified diff identifying the source file.
- `task check` runs formatting validation before configuring the clang-tidy build and running its
  tests.
- The checked-in QML baseline passes `task format:check`.

## Non-goals

- Formatting files outside `src/` and `tests/`.
- Adding formatter dependencies or changing formatter configuration.
- Changing application behavior, public APIs, generated files, or dependency lockfiles.

## Verification

- Run `task format` twice and confirm the second run creates no tracked changes.
- Run `task format:check`.
- Confirm temporary formatting violations in one C++ file and one QML file are rejected, then
  restore both files.
- Run `task check` and `git diff --check`.
