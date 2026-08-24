# HoloNight Dashboard

A standalone Qt/QML instrument-panel dashboard for Raspberry Pi 5. The first target is a 1480×320 landscape touchscreen running as a Wayland kiosk. It will display local metrics and complete telemetry snapshots sent by remote devices over UDP.

The repository currently provides the project foundation: a landscape dashboard shell based on the supplied mockups, a versioned telemetry domain type, a headless agent target, and protocol unit tests. Live metrics collection and UDP transport are intentionally not implemented yet.

## Requirements

- CMake 3.25 or newer
- Ninja
- A C++23 compiler
- Qt 6.8 or newer with Core, Gui, Network, Qml, Quick, and Test
- [Task](https://taskfile.dev/) for the convenience commands
- clang-tidy and clang-format for `task check` and `task format`

On Raspberry Pi OS/Debian, the relevant Qt packages are normally `qt6-base-dev`, `qt6-declarative-dev`, and `qt6-wayland`.

## Build and run

```sh
task build
task test
task run
```

Without Task:

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
./build/dev/src/dashboard/holonight-dashboard
```

Use `task check` for the clang-tidy build. If Qt is installed outside the system prefix, set `CMAKE_PREFIX_PATH` or `Qt6_ROOT` when configuring.

## Repository layout

```text
src/protocol/   GUI-free telemetry contract and serialization
src/agent/      Headless remote telemetry sender executable
src/dashboard/  Qt Quick dashboard executable and QML module
tests/          Fast protocol unit tests
docs/mockups/   Visual direction
docs/specs/     Feature specifications used by the SDD cycle
```

## Development approach

Features follow specification-driven development: define the user-visible behavior and acceptance criteria in `docs/specs/`, then implement the smallest vertical slice. Bugs follow red-green-refactor TDD and require a regression test before the fix.

The codebase uses only public Qt APIs and has no HoloNight runtime or build dependency. The visual language is represented by local semantic color and spacing tokens. See [AGENTS.md](AGENTS.md) for the full project rules and daily workflow.

## Near-term roadmap

1. Specify and implement local Linux metrics collection.
2. Specify the versioned UDP JSON envelope and receiver behavior.
3. Add device freshness/order handling and expose a registry model to QML.
4. Replace scaffold values with live models and add explicit systems detail views.
5. Add the Raspberry Pi kiosk service and deployment packaging.
