#pragma once

#include "protocol/system_info.h"
#include "protocol/system_metrics.h"

#include <QByteArray>
#include <QUuid>

#include <optional>
#include <variant>

namespace dashboard::protocol {

inline constexpr qint64 telemetry_protocol_version = 1;
inline constexpr qsizetype maximum_datagram_size = 16 * 1024;

struct Hello {
  QUuid device_id;
  QUuid instance_id;
  QString display_name;
  int interval_seconds{1};
  SystemInfo system_info;
};
struct RegistrationResult {
  QUuid device_id;
  QUuid instance_id;
  bool accepted{false};
  QString reason;
};
struct DeviceSnapshot {
  QUuid device_id;
  QUuid instance_id;
  int interval_seconds{1};
  quint64 sequence{0};
  SystemInfo system_info;
  std::optional<SystemMetrics> metrics;
};

using TelemetryMessage = std::variant<Hello, RegistrationResult, DeviceSnapshot>;
struct DecodeResult {
  std::optional<TelemetryMessage> message;
  QString error;
};

[[nodiscard]] QByteArray encodeMessage(const Hello& value);
[[nodiscard]] QByteArray encodeMessage(const RegistrationResult& value);
[[nodiscard]] QByteArray encodeMessage(const DeviceSnapshot& value);
[[nodiscard]] DecodeResult decodeMessage(const QByteArray& bytes);
[[nodiscard]] QByteArray encodeSystemInfo(const SystemInfo& info);
[[nodiscard]] std::optional<SystemInfo> decodeSystemInfo(const QByteArray& bytes);

}  // namespace dashboard::protocol
