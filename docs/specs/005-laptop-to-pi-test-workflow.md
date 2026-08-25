# Laptop-to-Raspberry-Pi Test Workflow

## Context

Most dashboard behavior should be testable on the development laptop, while Raspberry Pi graphics, input, and display integration still require the target hardware. Development needs an exact logical display geometry without changing kiosk defaults.

## Functional requirements

- The dashboard remains fullscreen by default.
- `--windowed` opens a normal development window.
- `--width` and `--height` select the windowed logical geometry and default to 1480×320.
- Non-positive or non-numeric dimensions are rejected with a nonzero exit status.
- Automated Qt Quick tests run at logical 1480×320 and cover page selection, bounded Left/Right navigation, Home, F5 focus, and layout bounds.
- Debug sanitizer presets provide AddressSanitizer and UndefinedBehaviorSanitizer builds.
- A developer can synchronize the source to a dedicated Raspberry Pi directory and run the complete native test suite over SSH.

## Acceptance criteria

- Starting without development options requests fullscreen presentation.
- `holonight-dashboard --windowed --width 1480 --height 320` creates a 1480×320 window.
- Keyboard navigation never wraps and F5 focuses the Overview chevron or the current placeholder on other pages.
- All meaningful dashboard content remains within the 1480×320 root bounds and sidebar buttons remain 48×48.
- `task test`, `task test-asan`, and `task test-ubsan` expose the corresponding local verification paths.
- `task test-pi PI_HOST=<host> PI_PATH=<dedicated-path>` runs `task test` from a synchronized source tree on the Pi.

## Non-goals

- Emulating the Pi GPU, DRM/KMS, Cage, touchscreen calibration, or rounded physical glass on the laptop.
- Cross-compiling Qt or the dashboard.
- Pixel-for-pixel screenshot comparison across different GPUs or font installations.
- Removing physical-device release validation.

## Verification

- Run the focused Qt Quick Test with the offscreen platform at scale factor 1.
- Run `task test`, followed by `task check`.
- During UI work, run `task run-windowed` and inspect all pages at 1480×320.
- Before merging, run `task test-pi` against the ARM64 target.
- After graphics or input changes, repeat the physical checklist in `docs/hardware-validation.md`.
