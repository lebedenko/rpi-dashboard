#pragma once

#include "sysmetrics/sys_metrics_collector.h"

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>

#include <memory>
#include <optional>

namespace dashboard::sysmetrics {

struct LinuxStorageVolume {
  QString mount_point;
  QString device_name;
  bool ready{false};
  bool read_only{false};
  quint64 total_bytes{0};
  quint64 available_bytes{0};
};

class LinuxSysMetricsAccess {
 public:
  LinuxSysMetricsAccess() = default;
  virtual ~LinuxSysMetricsAccess() = default;
  LinuxSysMetricsAccess(const LinuxSysMetricsAccess&) = default;
  LinuxSysMetricsAccess& operator=(const LinuxSysMetricsAccess&) = default;
  LinuxSysMetricsAccess(LinuxSysMetricsAccess&&) = default;
  LinuxSysMetricsAccess& operator=(LinuxSysMetricsAccess&&) = default;
  [[nodiscard]] virtual std::optional<QByteArray> readFile(const QString& path, qsizetype maximum_bytes) const = 0;
  [[nodiscard]] virtual QStringList directoryEntries(const QString& path) const = 0;
  [[nodiscard]] virtual QList<LinuxStorageVolume> storageVolumes() const = 0;
  [[nodiscard]] virtual qint64 monotonicMilliseconds() const = 0;
};

class LinuxSysMetricsCollector final : public SysMetricsCollector {
 public:
  struct Counters {
    quint64 active{0};
    quint64 total{0};
  };
  explicit LinuxSysMetricsCollector(std::shared_ptr<const LinuxSysMetricsAccess> access = {});
  [[nodiscard]] SysMetricsCollectionResult collect() override;

 private:
  std::shared_ptr<const LinuxSysMetricsAccess> access_;
  QHash<QString, Counters> cpu_counters_;
  QHash<QString, QPair<quint64, quint64>> network_counters_;
  qint64 previous_time_ms_{-1};
};

}  // namespace dashboard::sysmetrics
