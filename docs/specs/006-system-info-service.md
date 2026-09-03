# System Information Service

## Context and goal

The dashboard process needs a source-neutral description of its host and hardware. The description must be useful on
ordinary Linux systems and enriched on Raspberry Pi hardware without coupling it to telemetry, transport, or QML.

## Model

Every content field is optional because operating-system APIs and files may be absent or report unknown values.

| Group | Field | Classification | Meaning |
|---|---|---|---|
| Host | `host_name` | Baseline-expected | Configured non-FQDN hostname |
| OS | `os_family` | Baseline-expected | Normalized family such as `linux` or `macos` |
| OS | `os_id` | Platform-optional | Distribution or product identifier |
| OS | `os_version` | Platform-optional | Opaque product version |
| OS | `os_pretty_name` | Platform-optional | User-facing product name |
| Kernel | `kernel_type` | Baseline-expected | Kernel identifier such as `linux` or `darwin` |
| Kernel | `kernel_version` | Baseline-expected | Opaque kernel release |
| Hardware | `manufacturer` | Platform-optional | Hardware manufacturer |
| Hardware | `model` | Platform-optional, expected on Pi | Human-readable board or system model |
| Hardware | `board_revision` | Platform-optional | Opaque Raspberry Pi revision code |
| Hardware | `compatible_ids` | Platform-optional, expected on Pi | Ordered Device Tree compatible identifiers |
| CPU | `architecture` | Baseline-expected | Normalized runtime architecture |
| CPU | `vendor` | Platform-optional | CPU or SoC vendor |
| CPU | `model` | Platform-optional | CPU or SoC model |
| CPU | `logical_cpu_count` | Baseline-expected | Online logical processors visible to the OS |
| CPU | `physical_core_count` | Platform-optional | Reliably discovered physical cores |
| Memory | `total_bytes` | Baseline-expected | Total usable RAM visible to the OS |

Known architecture aliases include `amd64` to `x86_64` and `arm64` to `aarch64`. Empty strings, documented unknown
sentinels, zero counts, and zero byte values are absent rather than published.

## Collection and state

- Linux collection uses public Qt and POSIX APIs plus bounded reads of `/proc/meminfo`, Device Tree `model` and
  `compatible`, DMI `sys_vendor` and `product_name`, selected identity fields in `/proc/cpuinfo`, and Linux CPU
  topology in `/sys`.
- Generic Linux hardware manufacturer/model come from DMI when available. CPU vendor/model come only from exact
  `/proc/cpuinfo` keys (`vendor_id` and `model name`). Raspberry Pi Device Tree identity takes precedence over these
  generic sources.
- Physical cores are the distinct `(physical_package_id, core_id)` pairs for every CPU listed by
  `/sys/devices/system/cpu/online`. Single IDs and sparse ranges are accepted. The field is omitted unless the online
  list and every referenced topology value form a complete, positive, bounded result; SMT siblings therefore collapse
  to one physical core.
- Device Tree compatible values are NUL-separated, retain their source order, and identify a Pi only when a complete
  entry begins with `raspberrypi,`.
- `/proc/cpuinfo` serials and unrelated fields are never retained or exposed.
- Non-Pi Linux systems publish generic information. Missing Pi enrichment is diagnostic, not a collection failure.
- `SysInfoService` has `Idle`, `Collecting`, `Ready`, `Partial`, and `Error` states. It collects once at startup and on
  explicit refresh, off the GUI thread. Refresh requests during collection are coalesced into the active request.
- A record is `Ready` when all baseline fields are present, `Partial` when it contains any usable value but lacks a
  baseline field, and `Error` when collection yields no usable value.
- Ready and partial results replace the complete current snapshot and update the last-success UTC timestamp. A total
  failure preserves the previous successful snapshot and timestamp while replacing diagnostics.
- The service exposes QML-bindable scalar projections for the Overview device card. Optional fields remain invalid
  variants when absent; presentation formatting stays in QML.

## Privacy exclusions

The model excludes machine IDs, UUIDs, serial numbers, MAC addresses, boot IDs, and other stable identifiers. It also
excludes live state: uptime, utilization, frequency, temperatures, disks, interfaces, GPU, battery, firmware, and
virtualization status.

## Non-goals

- No JSON or UDP representation and no change to `DeviceSnapshot` protocol version.
- No direct QML type registration or presentation logic in the service.
- No remote collection, registry integration, periodic sampling, or freshness policy.
- No macOS production collector in this slice.

## Acceptance criteria

- Complete generic Linux and Raspberry Pi 5 fixtures produce normalized records and `Ready` service state.
- Missing baseline data produces `Partial`; missing optional Pi data never creates sentinels or invalid numeric values.
- Device Tree compatible ordering, Pi/SoC enrichment, architecture aliases, positive numeric validation, and bounded
  `MemTotal` conversion are deterministic and covered by tests.
- SMT, one-thread-per-core, and sparse online-CPU fixtures produce the expected physical-core count. Missing,
  malformed, duplicate, oversized, over-limit, or incomplete topology leaves `physical_core_count` absent.
- Non-Pi fixtures do not acquire Pi-derived fields, and sensitive fixture values cannot leak into `SystemInfo`.
- Generic DMI and processor identity are published when valid and omitted when missing, empty, `unknown`, or oversized.
- Tests observe asynchronous startup and refresh transitions, coalescing, and preservation after total failure.
