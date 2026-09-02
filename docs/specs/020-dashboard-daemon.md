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

Live metrics include strictly parsed physical and swap memory, CPU thermal-zone temperature, non-loopback network
counters and rates, `/` plus block-backed `/dev/*` mounts, and best-effort Raspberry Pi V3D metrics. V3D identity is
discovered from an explicit `.v3d` platform-device or devfreq entry. Utilization is derived from valid cumulative
`.v3d/gpu_stats` timestamp and runtime deltas. V3D core frequency comes only from a positive `.v3d` devfreq `cur_freq`; temperature comes
only from an explicitly GPU- or V3D-labelled thermal zone. On generic Linux hosts, known PCI GPU vendors are discovered
through DRM cards; cards with a readable utilization or Intel `gt_cur_freq_mhz` metric are ordered first. CPU thermal
collection accepts Raspberry Pi CPU/SoC labels and standard x86 package labels. Unsupported GPU values remain absent. Mounts
are deduplicated and sorted with `/` first, virtual filesystems are excluded, and the protocol limit of 64 is enforced.

## Installation and support

Each architecture-specific archive contains `dashboard-daemon`, `dashboard-daemon.service`, an
editable `config.toml`, and `install.sh` below a version-and-architecture directory. The root-only
POSIX installer rejects incompatible architectures, missing systemd, and invalid packaged or
preserved configuration before mutation. First installation atomically installs the files and
enables and starts the service. Upgrade preserves configuration, replaces the binary and unit,
and restarts it. Health requires a stable PID and restart count for five seconds. Upgrade failure
restores the prior binary, unit, enablement, and running state; first-install failure removes the
new runtime but keeps the edited configuration for diagnosis. Supported systems have Linux 5.4+
and systemd 235+.

## Acceptance criteria

- The standalone directory configures and tests without Qt.
- Configuration validation/discovery, UUID persistence, canonical map ordering, counter reset,
  and partial collection have deterministic tests.
- System information has protocol-v1 field parity with the dashboard collector: hostname domains are stripped;
  OS/kernel family and architecture aliases are normalized; Device Tree Raspberry Pi manufacturer, model, revision,
  ordered compatible IDs, and Broadcom CPU identity are enriched; and logical and physical online CPU counts are
  reported when topology is complete.
- Generic Linux hosts use bounded DMI `sys_vendor`/`product_name` reads and exact `/proc/cpuinfo` `vendor_id`/`model
  name` fields for hardware and processor descriptions. Raspberry Pi Device Tree values override generic identity.
- System-information reads are bounded. Empty or `unknown` text and zero, malformed, overflowing, oversized, or
  over-limit numeric/topology values are omitted without discarding valid fields from other sources.
- Field-level fixtures cover all 17 system-information fields, normalization, compatible-ID ordering, physical-core
  SMT collapse, partial snapshots, and the exclusion of `/proc/cpuinfo` serials and unrelated identifiers.
- Metrics fixtures cover valid and malformed swap, CPU thermal readings, non-loopback network counters and rates,
  V3D discovery and optional fields, real-mount filtering and ordering, invalid capacity relationships, and the
  64-volume bound.
- A loopback run registers and sends a snapshot accepted by the Qt protocol receiver.
- The service uses `DynamicUser`, `StateDirectory`, restricted filesystem access, journald, and
  restart-on-failure hardening.
- Installation validates before mutation, is atomic, preserves configuration on upgrade, starts
  on first install, and rolls runtime and systemd state back after failed upgrade health checks.
- `task install:daemon` performs a Release build before installing the daemon and service package.

## Non-goals

Authentication, encryption, discovery, Windows, macOS, 32-bit architectures, protocol redesign,
cross-architecture execution, dashboard deployment, and compatibility with pre-5.4 kernels or
pre-235 systemd are excluded.
