# Changelog

All notable changes to this project are documented in this file. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and releases follow
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.5] - 2026-09-04

### Added

- Refine screensaver solar events ([#11](https://github.com/lebedenko/rpi-dashboard/pull/11))
- Adjust layout according to mockup ([#13](https://github.com/lebedenko/rpi-dashboard/pull/13))
- Expose gpu swap disk and host telemetry ([#18](https://github.com/lebedenko/rpi-dashboard/pull/18))

### Fixed

- Recover after boot-time network failure ([#9](https://github.com/lebedenko/rpi-dashboard/pull/9))
- Preserve dashboard restarts ([#10](https://github.com/lebedenko/rpi-dashboard/pull/10))
- Improve frame rendering quality ([#12](https://github.com/lebedenko/rpi-dashboard/pull/12))
- Remediate review findings and enforce quality gates ([#14](https://github.com/lebedenko/rpi-dashboard/pull/14))
- Preserve curve geometry after layout ([#16](https://github.com/lebedenko/rpi-dashboard/pull/16))
- Smooth screensaver contrast treatment ([#17](https://github.com/lebedenko/rpi-dashboard/pull/17))
- Refine forecast layout ([#19](https://github.com/lebedenko/rpi-dashboard/pull/19))

### Changed

- Unify dashboard theme tokens ([#8](https://github.com/lebedenko/rpi-dashboard/pull/8))

## [0.1.4] - 2026-08-30

### Fixed

- Prevent empty daemon archives ([#6](https://github.com/lebedenko/rpi-dashboard/pull/6))

## [0.1.3] - 2026-08-30

### Fixed

- Split dashboard and daemon release packages ([#3](https://github.com/lebedenko/rpi-dashboard/pull/3))
- Run daemon release actions on host ([#4](https://github.com/lebedenko/rpi-dashboard/pull/4))

## [0.1.0] - 2026-08-28

### Added

- Standalone Qt Quick dashboard for local metrics, weather, GitHub project health, and remote
  versioned telemetry snapshots.
- Raspberry Pi 5 tty kiosk installation, configuration, screensaver, and telemetry daemon.
- Reproducible five-preset CI, checksummed source releases, and manually approved production
  deployment over Tailscale.

[0.1.5]: https://github.com/lebedenko/rpi-dashboard/compare/v0.1.4...v0.1.5
[0.1.4]: https://github.com/lebedenko/rpi-dashboard/compare/v0.1.3...v0.1.4
[0.1.3]: https://github.com/lebedenko/rpi-dashboard/compare/v0.1.0...v0.1.3
[0.1.0]: https://github.com/lebedenko/rpi-dashboard/releases/tag/v0.1.0
