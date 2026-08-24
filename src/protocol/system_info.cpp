#include "protocol/system_info.h"

namespace dashboard::protocol {

bool SystemInfo::hasAnyValue() const {
  return host.host_name.has_value() || os.os_family.has_value() || os.os_id.has_value() || os.os_version.has_value() ||
         os.os_pretty_name.has_value() || kernel.kernel_type.has_value() || kernel.kernel_version.has_value() ||
         hardware.manufacturer.has_value() || hardware.model.has_value() || hardware.board_revision.has_value() ||
         hardware.compatible_ids.has_value() || cpu.architecture.has_value() || cpu.vendor.has_value() ||
         cpu.model.has_value() || cpu.logical_cpu_count.has_value() || cpu.physical_core_count.has_value() ||
         memory.total_bytes.has_value();
}

bool SystemInfo::hasAllBaselineFields() const {
  return host.host_name.has_value() && os.os_family.has_value() && kernel.kernel_type.has_value() &&
         kernel.kernel_version.has_value() && cpu.architecture.has_value() && cpu.logical_cpu_count.has_value() &&
         memory.total_bytes.has_value();
}

}  // namespace dashboard::protocol
