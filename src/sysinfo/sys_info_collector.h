#pragma once

#include "protocol/system_info.h"

#include <QStringList>

namespace dashboard::sysinfo {

struct SysInfoCollectionResult {
  protocol::SystemInfo info;
  QStringList diagnostics;
};

class SysInfoCollector {
 public:
  SysInfoCollector() = default;
  virtual ~SysInfoCollector() = default;
  SysInfoCollector(const SysInfoCollector&) = default;
  SysInfoCollector& operator=(const SysInfoCollector&) = default;
  SysInfoCollector(SysInfoCollector&&) = default;
  SysInfoCollector& operator=(SysInfoCollector&&) = default;
  [[nodiscard]] virtual SysInfoCollectionResult collect() const = 0;
};

}  // namespace dashboard::sysinfo
