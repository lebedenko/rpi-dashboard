#include "protocol/device_snapshot.h"

#include <QJsonValue>

namespace dashboard::protocol {
namespace {

void addOptionalNumber(QJsonObject& object, const QString& key, const std::optional<double>& value) {
  if (value.has_value()) {
    object.insert(key, *value);
  }
}

std::optional<double> optionalNumber(const QJsonObject& object, const QString& key) {
  const QJsonValue value = object.value(key);
  if (value.isUndefined()) {
    return std::nullopt;
  }
  if (!value.isDouble()) {
    return std::nullopt;
  }
  return value.toDouble();
}

bool isRatio(const std::optional<double>& value) {
  return !value.has_value() || (*value >= 0.0 && *value <= 1.0);
}

}  // namespace

bool DeviceSnapshot::isValid() const {
  return protocol_version == current_protocol_version && !device_id.isEmpty() && !display_name.isEmpty() &&
         !boot_id.isEmpty() && isRatio(cpu_usage_ratio) && isRatio(memory_usage_ratio);
}

QJsonObject toJson(const DeviceSnapshot& snapshot) {
  QJsonObject metrics;
  addOptionalNumber(metrics, QStringLiteral("cpu_usage_ratio"), snapshot.cpu_usage_ratio);
  addOptionalNumber(metrics, QStringLiteral("memory_usage_ratio"), snapshot.memory_usage_ratio);
  addOptionalNumber(metrics, QStringLiteral("temperature_c"), snapshot.temperature_celsius);

  return {
      {QStringLiteral("protocol"), snapshot.protocol_version},
      {QStringLiteral("device_id"), snapshot.device_id},
      {QStringLiteral("display_name"), snapshot.display_name},
      {QStringLiteral("boot_id"), snapshot.boot_id},
      {QStringLiteral("sequence"), static_cast<qint64>(snapshot.sequence)},
      {QStringLiteral("uptime_s"), static_cast<qint64>(snapshot.uptime_seconds)},
      {QStringLiteral("metrics"), metrics},
  };
}

std::optional<DeviceSnapshot> fromJson(const QJsonObject& object) {
  const QJsonValue protocol = object.value(QStringLiteral("protocol"));
  const QJsonValue sequence = object.value(QStringLiteral("sequence"));
  const QJsonValue uptime = object.value(QStringLiteral("uptime_s"));
  const QJsonValue metrics_value = object.value(QStringLiteral("metrics"));
  if (!protocol.isDouble() || !sequence.isDouble() || !uptime.isDouble() || !metrics_value.isObject()) {
    return std::nullopt;
  }

  const qint64 sequence_value = sequence.toInteger(-1);
  const qint64 uptime_value = uptime.toInteger(-1);
  if (sequence_value < 0 || uptime_value < 0) {
    return std::nullopt;
  }

  const QJsonObject metrics = metrics_value.toObject();
  DeviceSnapshot snapshot{
      .protocol_version = protocol.toInt(),
      .device_id = object.value(QStringLiteral("device_id")).toString(),
      .display_name = object.value(QStringLiteral("display_name")).toString(),
      .boot_id = object.value(QStringLiteral("boot_id")).toString(),
      .sequence = static_cast<quint64>(sequence_value),
      .uptime_seconds = static_cast<quint64>(uptime_value),
      .cpu_usage_ratio = optionalNumber(metrics, QStringLiteral("cpu_usage_ratio")),
      .memory_usage_ratio = optionalNumber(metrics, QStringLiteral("memory_usage_ratio")),
      .temperature_celsius = optionalNumber(metrics, QStringLiteral("temperature_c")),
  };

  if (!snapshot.isValid()) {
    return std::nullopt;
  }
  return snapshot;
}

}  // namespace dashboard::protocol
