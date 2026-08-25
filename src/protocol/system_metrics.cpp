#include "protocol/system_metrics.h"

#include <cmath>
#include <ranges>

namespace dashboard::protocol {
namespace {
bool ratio(const std::optional<double>& value) {
  return value && std::isfinite(*value) && *value >= 0.0 && *value <= 1.0;
}
bool finiteNonnegative(const std::optional<double>& value) { return value && std::isfinite(*value) && *value >= 0.0; }
bool validBytes(const std::optional<quint64>& total, const std::optional<quint64>& available) {
  return total && available && *total > 0 && *available <= *total;
}
}  // namespace

bool SystemMetrics::hasAnyValue() const {
  return ratio(cpu.usage_ratio) || finiteNonnegative(cpu.temperature_celsius) || !cpu.logical_cpus.isEmpty() ||
         memory.total_bytes || memory.available_bytes || memory.swap_total_bytes || memory.swap_available_bytes ||
         finiteNonnegative(system.uptime_seconds) || finiteNonnegative(system.load_average_1m) ||
         finiteNonnegative(system.load_average_5m) || finiteNonnegative(system.load_average_15m) ||
         !storage_volumes.isEmpty() || !network_interfaces.isEmpty() || !gpus.isEmpty();
}

bool SystemMetrics::hasAllBaselineFields() const {
  const bool primary = std::ranges::any_of(storage_volumes, [](const StorageVolume& volume) {
    return volume.primary && validBytes(volume.total_bytes, volume.available_bytes);
  });
  return ratio(cpu.usage_ratio) && finiteNonnegative(system.uptime_seconds) &&
         validBytes(memory.total_bytes, memory.available_bytes) && primary;
}

}  // namespace dashboard::protocol
