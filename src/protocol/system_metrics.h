#pragma once

#include <QList>
#include <QMetaType>
#include <QString>
#include <QtTypes>

#include <optional>

namespace dashboard::protocol {

struct SystemMetrics {
  struct LogicalCpu {
    QString name;
    std::optional<double> usage_ratio;
    std::optional<quint64> frequency_hz;
  };
  struct Cpu {
    std::optional<double> usage_ratio;
    QList<LogicalCpu> logical_cpus;
    std::optional<double> temperature_celsius;
  } cpu;
  struct Memory {
    std::optional<quint64> total_bytes;
    std::optional<quint64> available_bytes;
    std::optional<quint64> swap_total_bytes;
    std::optional<quint64> swap_available_bytes;
  } memory;
  struct System {
    std::optional<double> uptime_seconds;
    std::optional<double> load_average_1m;
    std::optional<double> load_average_5m;
    std::optional<double> load_average_15m;
  } system;
  struct StorageVolume {
    QString mount_point;
    QString device_name;
    bool primary{false};
    bool read_only{false};
    std::optional<quint64> total_bytes;
    std::optional<quint64> available_bytes;
  };
  struct NetworkInterface {
    QString name;
    std::optional<quint64> rx_bytes;
    std::optional<quint64> tx_bytes;
    std::optional<double> rx_bytes_per_second;
    std::optional<double> tx_bytes_per_second;
  };
  struct Gpu {
    QString name;
    std::optional<double> usage_ratio;
    std::optional<quint64> memory_total_bytes;
    std::optional<quint64> memory_used_bytes;
    std::optional<quint64> core_clock_hz;
    std::optional<quint64> memory_clock_hz;
    std::optional<double> temperature_celsius;
  };

  QList<StorageVolume> storage_volumes;
  QList<NetworkInterface> network_interfaces;
  QList<Gpu> gpus;

  [[nodiscard]] bool hasAnyValue() const;
  [[nodiscard]] bool hasAllBaselineFields() const;
};

}  // namespace dashboard::protocol

Q_DECLARE_METATYPE(dashboard::protocol::SystemMetrics)
