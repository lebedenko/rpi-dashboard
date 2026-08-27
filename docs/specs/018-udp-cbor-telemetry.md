# UDP CBOR remote telemetry

## Goal

Headless Linux agents publish complete system-information and metrics snapshots to the dashboard over a small, versioned UDP protocol. The dashboard remembers known devices across restarts and exposes their current freshness without coupling transport code to QML.

## Wire contract

Protocol version 1 uses CBOR maps with UTF-8 string keys. Encoders sort map keys. A datagram is one complete message and is limited to 16 KiB.

- `hello` contains `version`, `type`, persistent 16-byte `device_id`, per-process 16-byte `instance_id`, `display_name`, `interval_s` (1–5), and partial non-empty `system_info`.
- `registration_result` echoes both identifiers and contains `accepted`; rejected messages may contain a bounded `reason`: `unsupported_version`, `invalid_registration`, `duplicate_device_id`, or `registry_full`.
- `snapshot` contains both identifiers, `interval_s`, a nonnegative signed-64-bit `sequence`, complete current `system_info`, and optional complete current `metrics`.

The sender retries hello every 10 seconds. Snapshots are neither acknowledged nor retransmitted. Sequence starts at zero for every process instance and only strictly newer snapshots from the registered instance and source endpoint are accepted. A hello may move the same instance to a new endpoint. A new instance from the same IP replaces the old instance; another IP cannot claim an online identity until it is offline.

Known fields are strictly typed and range checked. Unknown bounded keys are ignored. Strings are at most 256 UTF-8 bytes (display names 128), and collections are limited to 256 logical CPUs, 64 storage volumes, 64 network interfaces, 32 compatible IDs, and 16 GPUs. Ratios are finite and in `[0, 1]`; byte counters and frequencies fit the nonnegative signed-64-bit range. Available memory cannot exceed total memory. Named collection entries must be non-empty and unique. Malformed CBOR, trailing bytes, duplicate keys, invalid relationships, unsupported messages, and over-limit datagrams are rejected atomically.

## Registry and freshness

A valid hello creates `Registered`; its first accepted snapshot makes it `Online`. Receiver monotonic arrival time is authoritative. At age `>= 3 × interval` a device is `Stale`, and at `>= 10 × interval` it is `Offline`. An accepted snapshot replaces the previous snapshot; omitted metrics clear previous metrics.

At most 64 devices are persisted as versioned CBOR through `QSaveFile` in the dashboard application-data directory. Persistence includes stable ordering, identity, display name, interval, and last valid system information. Metrics, endpoint, instance, and online state are never persisted. Loaded devices start offline. Corruption yields an empty registry and a diagnostic; save failures do not undo accepted telemetry. `forgetDevice` supports future management UI.

The dashboard listens non-blockingly in bounded event-loop batches on IPv4 `0.0.0.0:51337`, configurable with `--telemetry-bind-address` and `--telemetry-port`. Bind failure is nonfatal and diagnostic. Packet contents are never logged and malformed diagnostics are rate limited.

## Agent behavior

`dashboard-daemon` reads its destination and one-to-five-second interval from strict TOML configuration. Its UUID is atomically persisted in its systemd state directory; failure to establish persistent identity is fatal. Each process creates a new instance UUID. Display name defaults to the collected hostname. DNS and transient network failures are retried. The daemon waits for accepted registration before snapshots. A failed metrics attempt produces a snapshot without metrics, never stale values. See specification 020 for service and packaging details.

## Acceptance criteria

- Complete and partial model values round-trip through deterministic CBOR; malformed, out-of-range, duplicate, oversized, and trailing input is rejected without state change.
- Registration identity, source endpoint, cadence, sequence ordering, collision, replacement, capacity, and exact freshness boundaries behave as specified.
- Registry persistence reloads devices offline and excludes session/live fields; corrupt or unwritable storage is nonfatal and diagnostic.
- Loopback UDP registration returns acceptance/rejection and only valid registered snapshots update the registry; bind failure does not terminate the dashboard.
- The daemon validates configuration, persists identity, retries registration, and sends complete replacement snapshots at the configured cadence.

## Security boundary and non-goals

UDP input is untrusted. Validation, size/count limits, bounded processing, and concise diagnostics are the denial-of-service boundary. Version 1 intentionally provides no authentication, encryption, authorization, discovery/broadcast/mDNS, fragmentation, snapshot acknowledgement, remote history, or QML device model.
