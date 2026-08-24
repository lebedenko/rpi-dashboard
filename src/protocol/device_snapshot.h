#pragma once

#include <QJsonObject>
#include <QString>

#include <optional>

namespace dashboard::protocol {

struct DeviceSnapshot {
  static constexpr int current_protocol_version = 1;

  int protocol_version{current_protocol_version};
  QString device_id;
  QString display_name;
  QString boot_id;
  quint64 sequence{0};
  quint64 uptime_seconds{0};
  std::optional<double> cpu_usage_ratio;
  std::optional<double> memory_usage_ratio;
  std::optional<double> temperature_celsius;

  [[nodiscard]] bool isValid() const;
};

[[nodiscard]] QJsonObject toJson(const DeviceSnapshot& snapshot);
[[nodiscard]] std::optional<DeviceSnapshot> fromJson(const QJsonObject& object);

}  // namespace dashboard::protocol

