# Static dashboard telemetry daemon

## Goal

`dashboard-daemon` is an independently buildable Linux C++23 service which sends protocol-v1
telemetry without Qt, Python, external commands, or runtime libraries. Release builds are static
musl executables for x86_64 and aarch64.

## Configuration and lifecycle

The daemon accepts `--config PATH`, `--check-config`, `--help`, and `--version`. Without an
explicit path it checks `$XDG_CONFIG_HOME`, each entry in `$XDG_CONFIG_DIRS`, then
`/etc/xdg/dashboard-daemon/config.toml`. Configuration is read once at startup:

```toml
[dashboard]
host = ""
port = 51337

[telemetry]
interval_seconds = 1
display_name = ""
```

The parser accepts only these scalar keys and rejects invalid UTF-8, unknown or duplicate keys,
malformed values, ports outside 1..65535, intervals outside 1..5, and an empty host. An empty
display name uses the hostname. The stable device UUID lives at
`/var/lib/dashboard-daemon/device-id`; every process generates a fresh instance UUID.

The daemon collects optional current fields directly from `/proc`, `/sys`, `statvfs`, `uname`, and
`/etc/os-release`. A failed source omits its fields. It sends a canonical-CBOR hello every ten
seconds until accepted and then complete snapshots at the configured interval. DNS and transient
UDP errors are retried. Invalid configuration, persistent identity failure, unsupported protocol,
and explicit rejection are fatal.

## Installation and support

The archive contains `dashboard-daemon`, `dashboard-daemon.service`, `config.toml`, and
`install.sh`. The root-only POSIX installer supports x86_64/aarch64 systemd hosts, installs below
`/usr/local` and `/etc/xdg`, preserves existing configuration, reloads systemd, and leaves the
service stopped and disabled. Supported systems have Linux 5.4+ and systemd 235+.

## Acceptance criteria

- The standalone directory configures and tests without Qt.
- Configuration validation/discovery, UUID persistence, canonical map ordering, counter reset,
  and partial collection have deterministic tests.
- A loopback run registers and sends a snapshot accepted by the Qt protocol receiver.
- The service uses `DynamicUser`, `StateDirectory`, restricted filesystem access, journald, and
  restart-on-failure hardening.
- Installation is atomic, preserves configuration, validates its host, and does not enable or
  start the service.

## Non-goals

Authentication, encryption, discovery, Windows, macOS, 32-bit architectures, protocol redesign,
and compatibility with pre-5.4 kernels or pre-235 systemd are excluded.
