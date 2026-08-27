# rpi-dashboard

[![CI](https://github.com/lebedenko/rpi-dashboard/actions/workflows/ci.yml/badge.svg)](https://github.com/lebedenko/rpi-dashboard/actions/workflows/ci.yml)

A standalone Qt/QML instrument-panel dashboard for Raspberry Pi 5. The first target is a 1480×320 landscape touchscreen running as a Wayland kiosk. It will display local metrics and complete telemetry snapshots sent by remote devices over UDP.

The dashboard collects local Linux system information and metrics and accepts complete versioned CBOR telemetry snapshots from remote agents over UDP. Known remote devices survive dashboard restarts and are tracked through registered, online, stale, and offline states.

## Requirements

- CMake 3.25 or newer
- Ninja
- A C++23 compiler
- Qt 6.8 or newer with Core, Gui, Network, Qml, Quick, and Test
- [Task](https://taskfile.dev/) for the convenience commands
- clang-tidy and clang-format for `task check` and `task format`

On Raspberry Pi OS/Debian, the relevant Qt packages are normally `qt6-base-dev`, `qt6-declarative-dev`, `qt6-svg-plugins`, `qt6-wayland`, and `libxkbcommon-dev`. The SVG image-format plugin is required to decode the embedded sidebar icons. Running directly from a TTY also requires [Cage](https://github.com/cage-kiosk/cage) and `wlr-randr`. Private Qt development packages are neither required nor permitted by the project architecture.

## Build and run

```sh
task build
task test
task run
task run-windowed
```

Start a remote sender with `./build/dev/src/agent/rpi-dashboard-agent --dashboard-host <dashboard-address>`.
The dashboard listens on IPv4 UDP port 51337 by default; use `--telemetry-bind-address` and
`--telemetry-port` to change the listener. Agent cadence is one second by default and may be set to
1–5 seconds with `--interval`.

For a copy-and-run sender on any Linux x86_64 host with Python 3.8 or newer, copy the single
dependency-free script and run:

```sh
python3 rpi-dashboard-telemetry.py --dashboard-host <dashboard-address>
```

The script stores a stable device UUID below `$XDG_DATA_HOME/rpi-dashboard/rpi-dashboard-agent`
(or `~/.local/share/...`) and accepts `--dashboard-port`, `--interval 1..5`, `--display-name`,
`--device-id`, and `--once`. From this checkout the equivalent Task command is:

```sh
task run-python-agent DASHBOARD_HOST=<dashboard-address> DISPLAY_NAME=my-server
```

Without Task:

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
./build/dev/src/dashboard/rpi-dashboard
```

Use `task check` for the clang-tidy build. If Qt is installed outside the system prefix, set `CMAKE_PREFIX_PATH` or `Qt6_ROOT` when configuring.

Use `task ci` for the authoritative pre-push check in the digest-locked project container, or
`scripts/run-ci-container.sh tidy` for a targeted containerized clang-tidy run. The initial image
publication and digest-lock procedure is documented in
[the reproducible CI toolchain specification](docs/specs/016-reproducible-ci-toolchain.md).

For exact-geometry laptop UI work, `task run-windowed` opens a normal 1480×320 window; production launches remain fullscreen. `task test-asan` and `task test-ubsan` run separate sanitizer builds. `QT_SCALE_FACTOR` may be varied for extra DPI robustness checks, but the primary geometry is scale 1 at logical 1480×320.

To run the complete suite natively on an SSH-accessible ARM64 Raspberry Pi, choose a dedicated remote source/build directory and run:

```sh
task test-pi PI_HOST=dashboard-pi.local PI_PATH=/home/dashboard/rpi-dashboard-test
```

The task synchronizes source files without deleting remote files, excludes local build output, and invokes `task test` on the Pi. It does not replace the physical graphics and input checks below.

## Raspberry Pi TTY kiosk

Log in on an active local Raspberry Pi TTY, then build natively and start Cage as that non-root user with the dashboard as its fullscreen Wayland client:

```sh
cmake --preset release
cmake --build --preset release --target rpi-dashboard
./scripts/run-kiosk.sh
```

The launcher accepts an alternate dashboard executable as its first argument. It reuses `XDG_RUNTIME_DIR` when the login session provides one; otherwise it creates a private mode-0700 runtime directory under `/tmp`. Its Cage session helper applies the validated `HDMI-A-1` transform before starting Qt. Missing executables, missing `wlr-randr`, failed output configuration, and startup diagnostics are written to the TTY and produce a nonzero exit status.

For first-device validation, launch with `QSG_INFO=1 ./scripts/run-kiosk.sh`. Confirm the dashboard fills the 1480×320 panel, all four touch targets work, Left/Right/Home navigation does not wrap, F5 visibly focuses the current placeholder, and no Wayland platform or QML module errors appear. Switch to another VT to stop the process and confirm that the console is recovered. The validated Waveshare output, touch calibration, seat requirements, and rounded-corner safe-area constraint are documented in [the Raspberry Pi 5 hardware validation report](docs/hardware-validation.md).

## Repository layout

```text
src/protocol/   GUI-free telemetry contract and serialization
src/telemetry/  UDP receiver and persistent remote-device registry
src/agent/      Headless remote telemetry sender executable
src/dashboard/  Qt Quick dashboard executable and QML module
tests/          Protocol unit tests and dashboard startup integration test
docs/mockups/   Visual direction
docs/specs/     Feature specifications used by the SDD cycle
docs/hardware-validation.md  Target hardware validation and operational notes
```

## Development approach

Features follow specification-driven development: define the user-visible behavior and acceptance criteria in `docs/specs/`, then implement the smallest vertical slice. Bugs follow red-green-refactor TDD and require a regression test before the fix.

The codebase uses only public Qt APIs and has no HoloNight runtime or build dependency. The visual language is represented by local semantic color and spacing tokens. See [AGENTS.md](AGENTS.md) for the full project rules and daily workflow.

## Near-term roadmap

1. Add remote-device management UI.
2. Add optional authenticated transport without weakening the local-only default.
