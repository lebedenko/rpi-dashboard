#pragma once

#include "protocol/system_info.h"
#include "protocol/system_metrics.h"

#include <QAbstractListModel>
#include <QPointer>

namespace dashboard::sysinfo {
class SysInfoService;
}
namespace dashboard::sysmetrics {
class SysMetricsService;
}
namespace dashboard::telemetry {
class RemoteDeviceRegistry;
}

namespace dashboard {

class DeviceModel final : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

 public:
  enum Role {  // NOLINT(cppcoreguidelines-use-enum-class, performance-enum-size)
    DeviceNumberRole = Qt::UserRole + 1,
    DeviceIdRole,
    HostnameRole,
    StatusKeyRole,
    LocalRole,
    HistoryAvailableRole,
    CpuMetricRole,
    MemoryMetricRole,
    CpuUsageRatioRole,
    MemoryUsageRatioRole,
    TemperatureMetricRole,
    UptimeMetricRole,
    UptimeSecondsRole,
    CpuFrequencyHzRole,
    MemoryUsedBytesRole,
    SwapUsedBytesRole,
    DiskUsedBytesRole,
    DiskTotalBytesRole,
    DiskUsageRatioRole,
    BoardTemperatureCelsiusRole,
    GpuNameRole,
    GpuUsageRatioRole,
    GpuCoreClockHzRole,
    GpuTemperatureCelsiusRole,
    NetworkReceiveRateRole,
    NetworkTransmitRateRole,
    NetworkInterfaceNameRole,
    BootTimeMsRole,
    OsDescriptionRole,
    KernelDescriptionRole,
    ArchitectureRole,
    HardwareDescriptionRole,
    CpuDescriptionRole,
    CoreDescriptionRole,
    TotalMemoryRole
  };

  DeviceModel(sysinfo::SysInfoService& info, sysmetrics::SysMetricsService& metrics,
              telemetry::RemoteDeviceRegistry& registry, QObject* parent = nullptr);
  ~DeviceModel() override;
  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
  Q_INVOKABLE [[nodiscard]] QVariantMap get(int index) const;

 signals:
  void countChanged();

 private:
  Q_DISABLE_COPY_MOVE(DeviceModel)
  void localInfoChanged();
  void localMetricsChanged();
  void remoteChanged(const QUuid& device_id);
  void invalidateDependencies();
  QPointer<sysinfo::SysInfoService> info_;
  QPointer<sysmetrics::SysMetricsService> metrics_;
  QPointer<telemetry::RemoteDeviceRegistry> registry_;
  bool structured_registry_change_{false};
};

}  // namespace dashboard
