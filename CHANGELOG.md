# Changelog

All notable changes to this project are documented in this file. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and releases follow
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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

[0.1.3]: https://github.com/lebedenko/rpi-dashboard/compare/v0.1.0...v0.1.3
[0.1.0]: https://github.com/lebedenko/rpi-dashboard/releases/tag/v0.1.0
