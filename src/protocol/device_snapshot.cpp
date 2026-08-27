#include "protocol/device_snapshot.h"

#include <QCborArray>
#include <QCborMap>
#include <QCborParserError>
#include <QCborValue>
#include <QSet>

#include <cmath>
#include <limits>

namespace dashboard::protocol {
namespace {

// The codec intentionally keeps validation branches adjacent to their wire fields.
// NOLINTBEGIN(readability-braces-around-statements, readability-identifier-length,
// readability-isolate-declaration, readability-function-cognitive-complexity,
// modernize-use-designated-initializers)

constexpr quint64 maxInteger = static_cast<quint64>(std::numeric_limits<qint64>::max());

bool validText(const QString& text, qsizetype limit = 256) { return !text.isEmpty() && text.toUtf8().size() <= limit; }
void add(QCborMap& map, const char* key, const std::optional<QString>& value) {
  if (value) map.insert(QLatin1String(key), *value);
}
void add(QCborMap& map, const char* key, const std::optional<quint32>& value) {
  if (value) map.insert(QLatin1String(key), static_cast<qint64>(*value));
}
void add(QCborMap& map, const char* key, const std::optional<quint64>& value) {
  if (value && *value <= maxInteger) map.insert(QLatin1String(key), static_cast<qint64>(*value));
}
void add(QCborMap& map, const char* key, const std::optional<double>& value) {
  if (value) map.insert(QLatin1String(key), *value);
}

bool text(const QCborMap& map, const char* key, std::optional<QString>& output) {
  const auto value = map.value(QLatin1String(key));
  if (value.isUndefined()) return true;
  if (!value.isString() || !validText(value.toString())) return false;
  output = value.toString();
  return true;
}
bool integer(const QCborMap& map, const char* key, std::optional<quint64>& output) {
  const auto value = map.value(QLatin1String(key));
  if (value.isUndefined()) return true;
  if (!value.isInteger() || value.toInteger() < 0) return false;
  output = static_cast<quint64>(value.toInteger());
  return true;
}
bool count(const QCborMap& map, const char* key, std::optional<quint32>& output) {
  std::optional<quint64> value;
  if (!integer(map, key, value) || (value && *value > std::numeric_limits<quint32>::max())) return false;
  if (value) output = static_cast<quint32>(*value);
  return true;
}
bool number(const QCborMap& map, const char* key, std::optional<double>& output, bool ratio = false) {
  const auto value = map.value(QLatin1String(key));
  if (value.isUndefined()) return true;
  if (!value.isDouble() && !value.isInteger()) return false;
  const double result = value.toDouble();
  if (!std::isfinite(result) || (ratio && (result < 0.0 || result > 1.0))) return false;
  output = result;
  return true;
}

QCborMap infoToMap(const SystemInfo& info) {
  QCborMap host;
  add(host, "host_name", info.host.host_name);
  QCborMap os;
  add(os, "os_family", info.os.os_family);
  add(os, "os_id", info.os.os_id);
  add(os, "os_version", info.os.os_version);
  add(os, "os_pretty_name", info.os.os_pretty_name);
  QCborMap kernel;
  add(kernel, "kernel_type", info.kernel.kernel_type);
  add(kernel, "kernel_version", info.kernel.kernel_version);
  QCborMap hardware;
  add(hardware, "manufacturer", info.hardware.manufacturer);
  add(hardware, "model", info.hardware.model);
  add(hardware, "board_revision", info.hardware.board_revision);
  if (info.hardware.compatible_ids) {
    QCborArray values;
    for (const auto& value : *info.hardware.compatible_ids) values.append(value);
    hardware.insert(QStringLiteral("compatible_ids"), values);
  }
  QCborMap cpu;
  add(cpu, "architecture", info.cpu.architecture);
  add(cpu, "vendor", info.cpu.vendor);
  add(cpu, "model", info.cpu.model);
  add(cpu, "logical_cpu_count", info.cpu.logical_cpu_count);
  add(cpu, "physical_core_count", info.cpu.physical_core_count);
  QCborMap memory;
  add(memory, "total_bytes", info.memory.total_bytes);
  QCborMap result;
  if (!host.isEmpty()) result.insert(QStringLiteral("host"), host);
  if (!os.isEmpty()) result.insert(QStringLiteral("os"), os);
  if (!kernel.isEmpty()) result.insert(QStringLiteral("kernel"), kernel);
  if (!hardware.isEmpty()) result.insert(QStringLiteral("hardware"), hardware);
  if (!cpu.isEmpty()) result.insert(QStringLiteral("cpu"), cpu);
  if (!memory.isEmpty()) result.insert(QStringLiteral("memory"), memory);
  return result;
}

std::optional<SystemInfo> infoFromMap(const QCborValue& value) {
  if (!value.isMap()) return std::nullopt;
  const auto root = value.toMap();
  SystemInfo info;
  QCborMap host, os, kernel, hardware, cpu, memory;
  auto section = [&](const char* key, QCborMap& output) {
    const auto found = root.value(QLatin1String(key));
    if (found.isUndefined()) return true;
    if (!found.isMap()) return false;
    output = found.toMap();
    return true;
  };
  if (!section("host", host) || !section("os", os) || !section("kernel", kernel) || !section("hardware", hardware) ||
      !section("cpu", cpu) || !section("memory", memory))
    return std::nullopt;
  if (!text(host, "host_name", info.host.host_name) || !text(os, "os_family", info.os.os_family) ||
      !text(os, "os_id", info.os.os_id) || !text(os, "os_version", info.os.os_version) ||
      !text(os, "os_pretty_name", info.os.os_pretty_name) || !text(kernel, "kernel_type", info.kernel.kernel_type) ||
      !text(kernel, "kernel_version", info.kernel.kernel_version) ||
      !text(hardware, "manufacturer", info.hardware.manufacturer) || !text(hardware, "model", info.hardware.model) ||
      !text(hardware, "board_revision", info.hardware.board_revision) ||
      !text(cpu, "architecture", info.cpu.architecture) || !text(cpu, "vendor", info.cpu.vendor) ||
      !text(cpu, "model", info.cpu.model) || !count(cpu, "logical_cpu_count", info.cpu.logical_cpu_count) ||
      !count(cpu, "physical_core_count", info.cpu.physical_core_count) ||
      !integer(memory, "total_bytes", info.memory.total_bytes))
    return std::nullopt;
  const auto rawIds = hardware.value(QStringLiteral("compatible_ids"));
  if (!rawIds.isUndefined()) {
    if (!rawIds.isArray() || rawIds.toArray().size() > 32) return std::nullopt;
    QStringList ids;
    QSet<QString> seen;
    for (const auto& raw : rawIds.toArray()) {
      if (!raw.isString() || !validText(raw.toString()) || seen.contains(raw.toString())) return std::nullopt;
      ids.append(raw.toString());
      seen.insert(raw.toString());
    }
    info.hardware.compatible_ids = ids;
  }
  if (info.cpu.logical_cpu_count && *info.cpu.logical_cpu_count > 256) return std::nullopt;
  return info.hasAnyValue() ? std::optional(info) : std::nullopt;
}

QCborMap metricsToMap(const SystemMetrics& value) {
  QCborMap cpu;
  add(cpu, "usage_ratio", value.cpu.usage_ratio);
  add(cpu, "temperature_celsius", value.cpu.temperature_celsius);
  if (!value.cpu.logical_cpus.isEmpty()) {
    QCborArray list;
    for (const auto& entry : value.cpu.logical_cpus) {
      QCborMap item{{QStringLiteral("name"), entry.name}};
      add(item, "usage_ratio", entry.usage_ratio);
      add(item, "frequency_hz", entry.frequency_hz);
      list.append(item);
    }
    cpu.insert(QStringLiteral("logical_cpus"), list);
  }
  QCborMap memory;
  add(memory, "total_bytes", value.memory.total_bytes);
  add(memory, "available_bytes", value.memory.available_bytes);
  add(memory, "swap_total_bytes", value.memory.swap_total_bytes);
  add(memory, "swap_available_bytes", value.memory.swap_available_bytes);
  QCborMap system;
  add(system, "uptime_seconds", value.system.uptime_seconds);
  add(system, "load_average_1m", value.system.load_average_1m);
  add(system, "load_average_5m", value.system.load_average_5m);
  add(system, "load_average_15m", value.system.load_average_15m);
  QCborMap root;
  if (!cpu.isEmpty()) root.insert(QStringLiteral("cpu"), cpu);
  if (!memory.isEmpty()) root.insert(QStringLiteral("memory"), memory);
  if (!system.isEmpty()) root.insert(QStringLiteral("system"), system);
  QCborArray storage;
  for (const auto& entry : value.storage_volumes) {
    QCborMap item{{QStringLiteral("mount_point"), entry.mount_point},
                  {QStringLiteral("device_name"), entry.device_name},
                  {QStringLiteral("primary"), entry.primary},
                  {QStringLiteral("read_only"), entry.read_only}};
    add(item, "total_bytes", entry.total_bytes);
    add(item, "available_bytes", entry.available_bytes);
    storage.append(item);
  }
  if (!storage.isEmpty()) root.insert(QStringLiteral("storage_volumes"), storage);
  QCborArray network;
  for (const auto& entry : value.network_interfaces) {
    QCborMap item{{QStringLiteral("name"), entry.name}};
    add(item, "rx_bytes", entry.rx_bytes);
    add(item, "tx_bytes", entry.tx_bytes);
    add(item, "rx_bytes_per_second", entry.rx_bytes_per_second);
    add(item, "tx_bytes_per_second", entry.tx_bytes_per_second);
    network.append(item);
  }
  if (!network.isEmpty()) root.insert(QStringLiteral("network_interfaces"), network);
  QCborArray gpus;
  for (const auto& entry : value.gpus) {
    QCborMap item{{QStringLiteral("name"), entry.name}};
    add(item, "usage_ratio", entry.usage_ratio);
    add(item, "memory_total_bytes", entry.memory_total_bytes);
    add(item, "memory_used_bytes", entry.memory_used_bytes);
    add(item, "core_clock_hz", entry.core_clock_hz);
    add(item, "memory_clock_hz", entry.memory_clock_hz);
    add(item, "temperature_celsius", entry.temperature_celsius);
    gpus.append(item);
  }
  if (!gpus.isEmpty()) root.insert(QStringLiteral("gpus"), gpus);
  return root;
}

std::optional<SystemMetrics> metricsFromMap(const QCborValue& value) {
  if (!value.isMap()) return std::nullopt;
  const auto root = value.toMap();
  SystemMetrics output;
  QCborMap cpu, memory, system;
  auto section = [&](const char* key, QCborMap& out) {
    auto raw = root.value(QLatin1String(key));
    if (raw.isUndefined()) return true;
    if (!raw.isMap()) return false;
    out = raw.toMap();
    return true;
  };
  if (!section("cpu", cpu) || !section("memory", memory) || !section("system", system) ||
      !number(cpu, "usage_ratio", output.cpu.usage_ratio, true) ||
      !number(cpu, "temperature_celsius", output.cpu.temperature_celsius) ||
      !integer(memory, "total_bytes", output.memory.total_bytes) ||
      !integer(memory, "available_bytes", output.memory.available_bytes) ||
      !integer(memory, "swap_total_bytes", output.memory.swap_total_bytes) ||
      !integer(memory, "swap_available_bytes", output.memory.swap_available_bytes) ||
      !number(system, "uptime_seconds", output.system.uptime_seconds) ||
      !number(system, "load_average_1m", output.system.load_average_1m) ||
      !number(system, "load_average_5m", output.system.load_average_5m) ||
      !number(system, "load_average_15m", output.system.load_average_15m))
    return std::nullopt;
  auto named = [&](const QCborValue& raw, qsizetype limit, auto parse) {
    if (raw.isUndefined()) return true;
    if (!raw.isArray() || raw.toArray().size() > limit) return false;
    QSet<QString> seen;
    for (const auto& value : raw.toArray()) {
      if (!value.isMap() || !parse(value.toMap(), seen)) return false;
    }
    return true;
  };
  if (!named(cpu.value(QStringLiteral("logical_cpus")), 256,
             [&](const QCborMap& item, QSet<QString>& seen) {
               auto name = item.value(QStringLiteral("name"));
               SystemMetrics::LogicalCpu entry;
               if (!name.isString() || !validText(name.toString()) || seen.contains(name.toString())) return false;
               entry.name = name.toString();
               seen.insert(entry.name);
               if (!number(item, "usage_ratio", entry.usage_ratio, true) ||
                   !integer(item, "frequency_hz", entry.frequency_hz))
                 return false;
               output.cpu.logical_cpus.append(entry);
               return true;
             }) ||
      !named(root.value(QStringLiteral("storage_volumes")), 64,
             [&](const QCborMap& item, QSet<QString>& seen) {
               auto name = item.value(QStringLiteral("mount_point"));
               auto device = item.value(QStringLiteral("device_name"));
               auto primary = item.value(QStringLiteral("primary"));
               auto ro = item.value(QStringLiteral("read_only"));
               SystemMetrics::StorageVolume entry;
               if (!name.isString() || !device.isString() || !primary.isBool() || !ro.isBool() ||
                   !validText(name.toString()) || !validText(device.toString()) || seen.contains(name.toString()))
                 return false;
               entry.mount_point = name.toString();
               entry.device_name = device.toString();
               entry.primary = primary.toBool();
               entry.read_only = ro.toBool();
               seen.insert(entry.mount_point);
               if (!integer(item, "total_bytes", entry.total_bytes) ||
                   !integer(item, "available_bytes", entry.available_bytes))
                 return false;
               output.storage_volumes.append(entry);
               return true;
             }) ||
      !named(root.value(QStringLiteral("network_interfaces")), 64,
             [&](const QCborMap& item, QSet<QString>& seen) {
               auto name = item.value(QStringLiteral("name"));
               SystemMetrics::NetworkInterface entry;
               if (!name.isString() || !validText(name.toString()) || seen.contains(name.toString())) return false;
               entry.name = name.toString();
               seen.insert(entry.name);
               if (!integer(item, "rx_bytes", entry.rx_bytes) || !integer(item, "tx_bytes", entry.tx_bytes) ||
                   !number(item, "rx_bytes_per_second", entry.rx_bytes_per_second) ||
                   !number(item, "tx_bytes_per_second", entry.tx_bytes_per_second))
                 return false;
               output.network_interfaces.append(entry);
               return true;
             }) ||
      !named(root.value(QStringLiteral("gpus")), 16, [&](const QCborMap& item, QSet<QString>& seen) {
        auto name = item.value(QStringLiteral("name"));
        SystemMetrics::Gpu entry;
        if (!name.isString() || !validText(name.toString()) || seen.contains(name.toString())) return false;
        entry.name = name.toString();
        seen.insert(entry.name);
        if (!number(item, "usage_ratio", entry.usage_ratio, true) ||
            !integer(item, "memory_total_bytes", entry.memory_total_bytes) ||
            !integer(item, "memory_used_bytes", entry.memory_used_bytes) ||
            !integer(item, "core_clock_hz", entry.core_clock_hz) ||
            !integer(item, "memory_clock_hz", entry.memory_clock_hz) ||
            !number(item, "temperature_celsius", entry.temperature_celsius))
          return false;
        output.gpus.append(entry);
        return true;
      }))
    return std::nullopt;
  if ((output.memory.total_bytes && output.memory.available_bytes &&
       *output.memory.available_bytes > *output.memory.total_bytes) ||
      (output.memory.swap_total_bytes && output.memory.swap_available_bytes &&
       *output.memory.swap_available_bytes > *output.memory.swap_total_bytes))
    return std::nullopt;
  for (const auto& item : output.storage_volumes)
    if (item.total_bytes && item.available_bytes && *item.available_bytes > *item.total_bytes) return std::nullopt;
  for (const auto& item : output.gpus)
    if (item.memory_total_bytes && item.memory_used_bytes && *item.memory_used_bytes > *item.memory_total_bytes)
      return std::nullopt;
  return output;
}

QCborMap envelope(const QString& type, const QUuid& device, const QUuid& instance) {
  return {{QStringLiteral("device_id"), device.toRfc4122()},
          {QStringLiteral("instance_id"), instance.toRfc4122()},
          {QStringLiteral("type"), type},
          {QStringLiteral("version"), telemetry_protocol_version}};
}
QByteArray encoded(const QCborMap& map) { return QCborValue(map).toCbor(QCborValue::SortKeysInMaps); }
std::optional<QUuid> id(const QCborMap& map, const char* key) {
  auto value = map.value(QLatin1String(key));
  if (!value.isByteArray() || value.toByteArray().size() != 16) return std::nullopt;
  auto result = QUuid::fromRfc4122(value.toByteArray());
  return result.isNull() ? std::nullopt : std::optional(result);
}
}  // namespace

QByteArray encodeMessage(const Hello& value) {
  auto map = envelope(QStringLiteral("hello"), value.device_id, value.instance_id);
  map.insert(QStringLiteral("display_name"), value.display_name);
  map.insert(QStringLiteral("interval_s"), value.interval_seconds);
  map.insert(QStringLiteral("system_info"), infoToMap(value.system_info));
  return encoded(map);
}
QByteArray encodeMessage(const RegistrationResult& value) {
  auto map = envelope(QStringLiteral("registration_result"), value.device_id, value.instance_id);
  map.insert(QStringLiteral("accepted"), value.accepted);
  if (!value.reason.isEmpty()) map.insert(QStringLiteral("reason"), value.reason);
  return encoded(map);
}
QByteArray encodeMessage(const DeviceSnapshot& value) {
  auto map = envelope(QStringLiteral("snapshot"), value.device_id, value.instance_id);
  map.insert(QStringLiteral("interval_s"), value.interval_seconds);
  map.insert(QStringLiteral("sequence"), static_cast<qint64>(value.sequence));
  map.insert(QStringLiteral("system_info"), infoToMap(value.system_info));
  if (value.metrics) map.insert(QStringLiteral("metrics"), metricsToMap(*value.metrics));
  return encoded(map);
}

DecodeResult decodeMessage(const QByteArray& bytes) {
  DecodeResult result;
  if (bytes.isEmpty() || bytes.size() > maximum_datagram_size) {
    result.error = QStringLiteral("invalid_size");
    return result;
  }
  QCborParserError error;
  auto value = QCborValue::fromCbor(bytes, &error);
  if (error.error != QCborError::NoError || !value.isMap()) {
    result.error = QStringLiteral("malformed_cbor");
    return result;
  }
  auto root = value.toMap();
  auto version = root.value(QStringLiteral("version"));
  auto type = root.value(QStringLiteral("type"));
  auto device = id(root, "device_id");
  auto instance = id(root, "instance_id");
  if (!version.isInteger() || version.toInteger() != telemetry_protocol_version) {
    result.error = QStringLiteral("unsupported_version");
    return result;
  }
  if (!type.isString() || !device || !instance) {
    result.error = QStringLiteral("invalid_identity");
    return result;
  }
  if (type.toString() == QStringLiteral("hello")) {
    auto name = root.value(QStringLiteral("display_name"));
    auto interval = root.value(QStringLiteral("interval_s"));
    auto info = infoFromMap(root.value(QStringLiteral("system_info")));
    if (!name.isString() || !validText(name.toString(), 128) || !interval.isInteger() || interval.toInteger() < 1 ||
        interval.toInteger() > 5 || !info) {
      result.error = QStringLiteral("invalid_registration");
      return result;
    }
    result.message = Hello{*device, *instance, name.toString(), static_cast<int>(interval.toInteger()), *info};
  } else if (type.toString() == QStringLiteral("registration_result")) {
    auto accepted = root.value(QStringLiteral("accepted"));
    auto reason = root.value(QStringLiteral("reason"));
    if (!accepted.isBool() ||
        (!reason.isUndefined() && (!reason.isString() || reason.toString().toUtf8().size() > 64))) {
      result.error = QStringLiteral("invalid_registration_result");
      return result;
    }
    result.message = RegistrationResult{*device, *instance, accepted.toBool(), reason.toString()};
  } else if (type.toString() == QStringLiteral("snapshot")) {
    auto interval = root.value(QStringLiteral("interval_s"));
    auto sequence = root.value(QStringLiteral("sequence"));
    auto info = infoFromMap(root.value(QStringLiteral("system_info")));
    if (!interval.isInteger() || interval.toInteger() < 1 || interval.toInteger() > 5 || !sequence.isInteger() ||
        sequence.toInteger() < 0 || !info) {
      result.error = QStringLiteral("invalid_snapshot");
      return result;
    }
    std::optional<SystemMetrics> metrics;
    auto raw = root.value(QStringLiteral("metrics"));
    if (!raw.isUndefined()) {
      metrics = metricsFromMap(raw);
      if (!metrics) {
        result.error = QStringLiteral("invalid_metrics");
        return result;
      }
    }
    result.message = DeviceSnapshot{
        *device, *instance, static_cast<int>(interval.toInteger()), static_cast<quint64>(sequence.toInteger()),
        *info,   metrics};
  } else {
    result.error = QStringLiteral("unsupported_type");
  }
  return result;
}
QByteArray encodeSystemInfo(const SystemInfo& info) { return encoded(infoToMap(info)); }
std::optional<SystemInfo> decodeSystemInfo(const QByteArray& bytes) {
  QCborParserError error;
  auto value = QCborValue::fromCbor(bytes, &error);
  return error.error == QCborError::NoError ? infoFromMap(value) : std::nullopt;
}

// NOLINTEND(readability-braces-around-statements, readability-identifier-length,
// readability-isolate-declaration, readability-function-cognitive-complexity,
// modernize-use-designated-initializers)

}  // namespace dashboard::protocol
