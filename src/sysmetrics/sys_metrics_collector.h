#pragma once

#include "protocol/system_metrics.h"

#include <QStringList>

namespace dashboard::sysmetrics {

struct SysMetricsCollectionResult {
  protocol::SystemMetrics metrics;
  QStringList diagnostics;
};

class SysMetricsCollector {
 public:
  SysMetricsCollector() = default;
  virtual ~SysMetricsCollector() = default;
  SysMetricsCollector(const SysMetricsCollector&) = default;
  SysMetricsCollector& operator=(const SysMetricsCollector&) = default;
  SysMetricsCollector(SysMetricsCollector&&) = default;
  SysMetricsCollector& operator=(SysMetricsCollector&&) = default;
  [[nodiscard]] virtual SysMetricsCollectionResult collect() = 0;
};

}  // namespace dashboard::sysmetrics
