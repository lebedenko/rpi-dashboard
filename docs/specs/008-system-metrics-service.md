# System Metrics Service

## Goal

Collect a source-neutral snapshot of live Linux system metrics every second without blocking the GUI thread. Raspberry Pi systems may add truthful sysfs enrichment; ordinary Linux systems remain supported.

## Observable contract

- `protocol::SystemMetrics` contains optional CPU utilization and clocks, memory and swap bytes, uptime and load averages, mounted-volume capacity, network counters and rates, and GPU-specific values.
- Units are explicit in field names. Ratios are finite and in `[0, 1]`; counters are nonnegative; available bytes never exceed totals.
- A snapshot is `Ready` when aggregate CPU usage, uptime, physical-memory total/available bytes, and primary-filesystem total/available bytes are present. A nonempty snapshot missing any baseline value is `Partial`; an empty snapshot is `Error`.
- Linux collection uses bounded local reads of `/proc` and documented sysfs files plus `QStorageInfo`. CPU utilization and network rates require a prior sample and are therefore absent on the first sample.
- Storage contains `/` plus ready block-backed `/dev/*` mounts, ordered with `/` first and then by mount point. Virtual filesystems and duplicate mount points are excluded, and at most 64 volumes are retained.
- Raspberry Pi V3D is discovered from explicit `.v3d` platform-device or devfreq entries. The collector publishes its `V3D` identity and, when exposed, a positive devfreq `cur_freq`; a temperature is published only from an explicitly GPU- or V3D-labelled thermal zone. Unsupported GPU utilization and memory values remain absent.
- Missing, malformed, reset, or disappearing optional sources omit only affected values and produce concise diagnostics.
- `SysMetricsService` samples off the GUI thread, starts immediately, defaults to a 1000 ms interval, prevents overlap, and coalesces refresh requests made while collecting.
- Ready and partial results replace the whole snapshot and update `lastSuccessUtc`. Errors preserve the last successful snapshot and timestamp.
- Every completed attempt is recorded in the non-persistent rolling usage-history model described in `009-live-resource-history.md`; failed or missing metrics are recorded as gaps.
- QML receives CPU usage, memory usage, CPU/SoC temperature, and uptime projections. The local card formats percentages as rounded integers, temperature as rounded Celsius, and uptime compactly. Missing values remain `—`.

## Non-goals

- No `DeviceSnapshot` JSON or UDP protocol change.
- No disk-I/O rates, SMART data, partition inventory, fan speed, throttling/undervoltage flags, external commands, or fabricated GPU metrics.
- Network and swap metrics have no UI in this slice.

## Acceptance criteria

- Deterministic tests cover model classification and validation, parsing and delta behavior, service lifecycle/replacement/failure/recovery/coalescing, and QML formatting/live updates.
- Generic Linux collection works without Raspberry Pi enrichment; GPU fields are emitted only from attributes with explicit GPU semantics. Malformed, zero, oversized, or unavailable GPU values omit only the affected field.
- Storage fixtures cover root selection, block-backed mount filtering, stable ordering, duplicate rejection, the 64-volume limit, and invalid capacity relationships.
- Metric updates do not replace the card or disturb its expansion, selection, or keyboard focus.
