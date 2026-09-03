#include "device_model.h"

#include "sysinfo/sys_info_service.h"
#include "sysmetrics/sys_metrics_service.h"
#include "telemetry/remote_device_registry.h"

#include <QDateTime>

#include <algorithm>

namespace dashboard {
// The role projection keeps each wire field adjacent to its QML role.
// NOLINTBEGIN(readability-braces-around-statements, cppcoreguidelines-narrowing-conversions,
// readability-implicit-bool-conversion, readability-function-cognitive-complexity,
// readability-identifier-length, readability-qualified-auto, performance-no-automatic-move)
namespace {
QVariant value(const auto& optional) { return optional ? QVariant::fromValue(*optional) : QVariant{}; }
QString joined(const std::optional<QString>& first, const std::optional<QString>& second) {
  QStringList parts;
  if (first) parts.append(*first);
  if (second) parts.append(*second);
  return parts.isEmpty() ? QStringLiteral("—") : parts.join(QStringLiteral(" · "));
}
QString cores(const protocol::SystemInfo& info) {
  if (info.cpu.physical_core_count && info.cpu.logical_cpu_count)
    return QStringLiteral("%1 physical · %2 logical")
        .arg(*info.cpu.physical_core_count)
        .arg(*info.cpu.logical_cpu_count);
  if (info.cpu.logical_cpu_count) return QStringLiteral("%1 logical").arg(*info.cpu.logical_cpu_count);
  if (info.cpu.physical_core_count) return QStringLiteral("%1 physical").arg(*info.cpu.physical_core_count);
  return QStringLiteral("—");
}
QVariant used(const std::optional<quint64>& total, const std::optional<quint64>& available) {
  return total && available ? QVariant::fromValue(*total - *available) : QVariant{};
}
QVariant ratio(const std::optional<quint64>& total, const std::optional<quint64>& available) {
  return total && available && *total > 0 ? QVariant::fromValue(static_cast<double>(*total - *available) / *total)
                                          : QVariant{};
}
QString percent(const QVariant& input) {
  return input.isValid() ? QStringLiteral("%1%").arg(qRound(input.toDouble() * 100)) : QStringLiteral("—");
}
QString temperature(const QVariant& input) {
  return input.isValid() ? QStringLiteral("%1°C").arg(qRound(input.toDouble())) : QStringLiteral("—");
}
QString uptime(const QVariant& input) {
  if (!input.isValid()) return QStringLiteral("—");
  const auto minutes = static_cast<qint64>(input.toDouble() / 60.0);
  if (minutes < 1) return QStringLiteral("<1m");
  if (minutes >= 1440) return QStringLiteral("%1d %2h").arg(minutes / 1440).arg((minutes / 60) % 24);
  if (minutes >= 60) return QStringLiteral("%1h %2m").arg(minutes / 60).arg(minutes % 60);
  return QStringLiteral("%1m").arg(minutes);
}
QVariant averageFrequency(const protocol::SystemMetrics& metrics) {
  quint64 sum = 0;
  quint64 count = 0;
  for (const auto& cpu : metrics.cpu.logical_cpus)
    if (cpu.frequency_hz) {
      sum += *cpu.frequency_hz;
      ++count;
    }
  return count ? QVariant::fromValue(sum / count) : QVariant{};
}
}  // namespace

DeviceModel::DeviceModel(sysinfo::SysInfoService& info, sysmetrics::SysMetricsService& metrics,
                         telemetry::RemoteDeviceRegistry& registry, QObject* parent)
    : QAbstractListModel(parent), info_(&info), metrics_(&metrics), registry_(&registry) {
  connect(&info, &sysinfo::SysInfoService::currentInfoChanged, this, &DeviceModel::localInfoChanged);
  connect(&metrics, &sysmetrics::SysMetricsService::currentMetricsChanged, this, &DeviceModel::localMetricsChanged);
  connect(&registry, &telemetry::RemoteDeviceRegistry::deviceChanged, this, &DeviceModel::remoteChanged);
  connect(&info, &QObject::destroyed, this, &DeviceModel::invalidateDependencies);
  connect(&metrics, &QObject::destroyed, this, &DeviceModel::invalidateDependencies);
  connect(&registry, &QObject::destroyed, this, &DeviceModel::invalidateDependencies);
  connect(&registry, &telemetry::RemoteDeviceRegistry::deviceAboutToBeAdded, this, [this](int remote_index) {
    structured_registry_change_ = true;
    beginInsertRows({}, remote_index + 1, remote_index + 1);
  });
  connect(&registry, &telemetry::RemoteDeviceRegistry::deviceAdded, this, [this] {
    endInsertRows();
    emit countChanged();
  });
  connect(&registry, &telemetry::RemoteDeviceRegistry::deviceAboutToBeRemoved, this, [this](int remote_index) {
    structured_registry_change_ = true;
    beginRemoveRows({}, remote_index + 1, remote_index + 1);
  });
  connect(&registry, &telemetry::RemoteDeviceRegistry::deviceRemoved, this, [this] {
    endRemoveRows();
    emit countChanged();
  });
  connect(&registry, &telemetry::RemoteDeviceRegistry::devicesChanged, this, [this] {
    if (structured_registry_change_) {
      structured_registry_change_ = false;
      return;
    }
    beginResetModel();
    endResetModel();
    emit countChanged();
  });
}

DeviceModel::~DeviceModel() = default;

int DeviceModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() || !info_ || !metrics_ || !registry_ ? 0 : 1 + registry_->devices().size();
}

QVariant DeviceModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) return {};
  const bool local = index.row() == 0;
  protocol::SystemInfo info = local ? info_->currentInfo() : registry_->devices().at(index.row() - 1).system_info;
  std::optional<protocol::SystemMetrics> remoteMetrics =
      local ? std::nullopt : registry_->devices().at(index.row() - 1).metrics;
  const protocol::SystemMetrics metrics =
      local ? metrics_->currentMetrics() : remoteMetrics.value_or(protocol::SystemMetrics{});
  const auto remote = local ? std::optional<telemetry::RemoteDeviceRegistry::Device>{}
                            : std::optional(registry_->devices().at(index.row() - 1));
  const QVariant cpuRatio = value(metrics.cpu.usage_ratio);
  const QVariant memoryRatio = ratio(metrics.memory.total_bytes, metrics.memory.available_bytes);
  const QVariant temp = value(metrics.cpu.temperature_celsius);
  const QVariant up = value(metrics.system.uptime_seconds);
  const auto gpu = metrics.gpus.isEmpty() ? nullptr : &metrics.gpus.first();
  const auto storage_iterator = std::ranges::find_if(
      metrics.storage_volumes, [](const protocol::SystemMetrics::StorageVolume& volume) { return volume.primary; });
  const auto storage = storage_iterator == metrics.storage_volumes.cend() ? nullptr : &*storage_iterator;
  const auto network = metrics.network_interfaces.isEmpty() ? nullptr : &metrics.network_interfaces.first();
  switch (role) {
    case DeviceNumberRole:
      return QStringLiteral("%1").arg(index.row() + 1, 2, 10, QLatin1Char('0'));
    case DeviceIdRole:
      return local ? QStringLiteral("local") : remote->device_id.toString(QUuid::WithoutBraces);
    case HostnameRole:
      return local ? value(info.host.host_name)
                   : QVariant(remote->display_name.isEmpty() ? value(info.host.host_name).toString()
                                                             : remote->display_name);
    case StatusKeyRole:
      if (local)
        return QStringLiteral("online");
      else
        switch (remote->state) {
          case telemetry::RemoteDeviceRegistry::State::Registered:
            return QStringLiteral("registered");
          case telemetry::RemoteDeviceRegistry::State::Online:
            return QStringLiteral("online");
          case telemetry::RemoteDeviceRegistry::State::Stale:
            return QStringLiteral("stale");
          case telemetry::RemoteDeviceRegistry::State::Offline:
            return QStringLiteral("offline");
        }
      break;
    case LocalRole:
    case HistoryAvailableRole:
      return local;
    case CpuMetricRole:
      return percent(cpuRatio);
    case MemoryMetricRole:
      return percent(memoryRatio);
    case CpuUsageRatioRole:
      return cpuRatio;
    case MemoryUsageRatioRole:
      return memoryRatio;
    case TemperatureMetricRole:
      return temperature(temp);
    case UptimeMetricRole:
      return uptime(up);
    case UptimeSecondsRole:
      return up;
    case CpuFrequencyHzRole:
      return averageFrequency(metrics);
    case MemoryUsedBytesRole:
      return used(metrics.memory.total_bytes, metrics.memory.available_bytes);
    case SwapUsedBytesRole:
      return used(metrics.memory.swap_total_bytes, metrics.memory.swap_available_bytes);
    case DiskUsedBytesRole:
      return storage ? used(storage->total_bytes, storage->available_bytes) : QVariant{};
    case DiskTotalBytesRole:
      return storage ? value(storage->total_bytes) : QVariant{};
    case DiskUsageRatioRole:
      return storage ? ratio(storage->total_bytes, storage->available_bytes) : QVariant{};
    case BoardTemperatureCelsiusRole:
      return temp;
    case GpuNameRole:
      return gpu && !gpu->name.isEmpty() ? QVariant::fromValue(gpu->name) : QVariant{};
    case GpuUsageRatioRole:
      return gpu ? value(gpu->usage_ratio) : QVariant{};
    case GpuCoreClockHzRole:
      return gpu ? value(gpu->core_clock_hz) : QVariant{};
    case GpuTemperatureCelsiusRole:
      return gpu ? value(gpu->temperature_celsius) : QVariant{};
    case NetworkReceiveRateRole:
      return network ? value(network->rx_bytes_per_second) : QVariant{};
    case NetworkTransmitRateRole:
      return network ? value(network->tx_bytes_per_second) : QVariant{};
    case NetworkInterfaceNameRole:
      return network ? network->name : QString{};
    case BootTimeMsRole: {
      if (local) return metrics_->bootTimeUtc();
      if (!remote->last_snapshot_utc.isValid() || !metrics.system.uptime_seconds) return {};
      return remote->last_snapshot_utc.addMSecs(-qRound64(*metrics.system.uptime_seconds * 1000.0)).toMSecsSinceEpoch();
    }
    case OsDescriptionRole:
      return joined(info.os.os_pretty_name, info.os.os_version);
    case KernelDescriptionRole:
      return joined(info.kernel.kernel_type, info.kernel.kernel_version);
    case ArchitectureRole:
      return value(info.cpu.architecture);
    case HardwareDescriptionRole:
      return joined(info.hardware.manufacturer, info.hardware.model);
    case CpuDescriptionRole:
      return joined(info.cpu.vendor, info.cpu.model);
    case CoreDescriptionRole:
      return cores(info);
    case TotalMemoryRole:
      return info.memory.total_bytes
                 ? QStringLiteral("%1 GiB").arg(static_cast<double>(*info.memory.total_bytes) / 1073741824.0, 0, 'f', 1)
                 : QStringLiteral("—");
  }
  return {};
}

QHash<int, QByteArray> DeviceModel::roleNames() const {
  return {{DeviceNumberRole, "deviceNumber"},
          {DeviceIdRole, "deviceId"},
          {HostnameRole, "hostname"},
          {StatusKeyRole, "statusKey"},
          {LocalRole, "local"},
          {HistoryAvailableRole, "historyAvailable"},
          {CpuMetricRole, "cpuMetric"},
          {MemoryMetricRole, "memoryMetric"},
          {CpuUsageRatioRole, "cpuUsageRatio"},
          {MemoryUsageRatioRole, "memoryUsageRatio"},
          {TemperatureMetricRole, "temperatureMetric"},
          {UptimeMetricRole, "uptimeMetric"},
          {UptimeSecondsRole, "uptimeSeconds"},
          {CpuFrequencyHzRole, "cpuFrequencyHz"},
          {MemoryUsedBytesRole, "memoryUsedBytes"},
          {SwapUsedBytesRole, "swapUsedBytes"},
          {DiskUsedBytesRole, "diskUsedBytes"},
          {DiskTotalBytesRole, "diskTotalBytes"},
          {DiskUsageRatioRole, "diskUsageRatio"},
          {BoardTemperatureCelsiusRole, "boardTemperatureCelsius"},
          {GpuNameRole, "gpuName"},
          {GpuUsageRatioRole, "gpuUsageRatio"},
          {GpuCoreClockHzRole, "gpuCoreClockHz"},
          {GpuTemperatureCelsiusRole, "gpuTemperatureCelsius"},
          {NetworkReceiveRateRole, "networkReceiveRate"},
          {NetworkTransmitRateRole, "networkTransmitRate"},
          {NetworkInterfaceNameRole, "networkInterfaceName"},
          {BootTimeMsRole, "bootTimeMs"},
          {OsDescriptionRole, "osDescription"},
          {KernelDescriptionRole, "kernelDescription"},
          {ArchitectureRole, "architecture"},
          {HardwareDescriptionRole, "hardwareDescription"},
          {CpuDescriptionRole, "cpuDescription"},
          {CoreDescriptionRole, "coreDescription"},
          {TotalMemoryRole, "totalMemory"}};
}
QVariantMap DeviceModel::get(int index) const {
  QVariantMap result;
  if (index < 0 || index >= rowCount()) return result;
  const auto roles = roleNames();
  for (auto it = roles.cbegin(); it != roles.cend(); ++it)
    result.insert(QString::fromLatin1(it.value()), data(this->index(index), it.key()));
  return result;
}
void DeviceModel::localInfoChanged() {
  if (rowCount() == 0) return;
  emit dataChanged(index(0), index(0),
                   {HostnameRole, OsDescriptionRole, KernelDescriptionRole, ArchitectureRole, HardwareDescriptionRole,
                    CpuDescriptionRole, CoreDescriptionRole, TotalMemoryRole});
}
void DeviceModel::localMetricsChanged() {
  if (rowCount() == 0) return;
  emit dataChanged(index(0), index(0),
                   {CpuMetricRole,
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
                    BootTimeMsRole});
}
void DeviceModel::remoteChanged(const QUuid& device_id) {
  const auto devices = registry_->devices();
  for (qsizetype i = 0; i < devices.size(); ++i)
    if (devices[i].device_id == device_id) {
      const auto roles = roleNames().keys();
      emit dataChanged(index(static_cast<int>(i + 1)), index(static_cast<int>(i + 1)), roles);
      return;
    }
}
void DeviceModel::invalidateDependencies() {
  beginResetModel();
  endResetModel();
  emit countChanged();
}
// NOLINTEND(readability-braces-around-statements, cppcoreguidelines-narrowing-conversions,
// readability-implicit-bool-conversion, readability-function-cognitive-complexity,
// readability-identifier-length, readability-qualified-auto, performance-no-automatic-move)
}  // namespace dashboard
