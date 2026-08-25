#include "sysmetrics/linux_sys_metrics_collector.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QStorageInfo>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace dashboard::sysmetrics {
namespace {
constexpr qsizetype kProcLimit = 1024 * 1024;
constexpr qsizetype kSysfsLimit = 4096;
constexpr quint64 kKib = 1024;

class NativeAccess final : public LinuxSysMetricsAccess {
 public:
  [[nodiscard]] std::optional<QByteArray> readFile(const QString& path, qsizetype maximum_bytes) const override {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
      return std::nullopt;
    }
    QByteArray value = file.read(maximum_bytes + 1);
    return value.size() <= maximum_bytes ? std::optional<QByteArray>(std::move(value)) : std::nullopt;
  }
  [[nodiscard]] QStringList directoryEntries(const QString& path) const override {
    return QDir(path).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
  }
  [[nodiscard]] QList<LinuxStorageVolume> storageVolumes() const override {
    QList<LinuxStorageVolume> result;
    for (const QStorageInfo& storage : QStorageInfo::mountedVolumes()) {
      const qint64 total = storage.bytesTotal();
      const qint64 available = storage.bytesAvailable();
      result.append({.mount_point = storage.rootPath(),
                     .device_name = QString::fromUtf8(storage.device()),
                     .ready = storage.isReady(),
                     .read_only = storage.isReadOnly(),
                     .total_bytes = total > 0 ? static_cast<quint64>(total) : 0,
                     .available_bytes = available >= 0 ? static_cast<quint64>(available) : 0});
    }
    return result;
  }
  [[nodiscard]] qint64 monotonicMilliseconds() const override {
    static QElapsedTimer timer = [] {
      QElapsedTimer value;
      value.start();
      return value;
    }();
    return timer.elapsed();
  }
};

std::optional<quint64> unsignedValue(const QByteArray& bytes) {
  bool valid = false;
  const quint64 value = bytes.trimmed().toULongLong(&valid);
  return valid ? std::optional<quint64>(value) : std::nullopt;
}

std::optional<double> nonnegativeDouble(const QByteArray& bytes) {
  bool valid = false;
  const double value = bytes.trimmed().toDouble(&valid);
  return valid && std::isfinite(value) && value >= 0.0 ? std::optional<double>(value) : std::nullopt;
}

QHash<QString, LinuxSysMetricsCollector::Counters> parseCpu(const QByteArray& bytes, QStringList& diagnostics) {
  QHash<QString, LinuxSysMetricsCollector::Counters> result;
  for (const QByteArray& line : bytes.split('\n')) {
    const QList<QByteArray> fields = line.simplified().split(' ');
    if (fields.isEmpty() || (fields[0] != "cpu" && !fields[0].startsWith("cpu"))) {
      continue;
    }
    if (fields.size() < 5) {
      diagnostics.append(QStringLiteral("Malformed /proc/stat CPU counters"));
      continue;
    }
    quint64 total = 0;
    quint64 idle = 0;
    bool valid = true;
    for (qsizetype index = 1; index < fields.size(); ++index) {
      const auto value = unsignedValue(fields[index]);
      if (!value || total > std::numeric_limits<quint64>::max() - *value) {
        valid = false;
        break;
      }
      total += *value;
      if (index == 4 || index == 5) {
        idle += *value;
      }
    }
    if (!valid || idle > total) {
      diagnostics.append(QStringLiteral("Invalid /proc/stat CPU counters"));
      continue;
    }
    result.insert(QString::fromLatin1(fields[0]), {.active = total - idle, .total = total});
  }
  return result;
}

std::optional<double> usage(const LinuxSysMetricsCollector::Counters& now,
                            const LinuxSysMetricsCollector::Counters& before, QStringList& diagnostics) {
  if (now.total <= before.total || now.active < before.active) {
    diagnostics.append(QStringLiteral("CPU counters reset"));
    return std::nullopt;
  }
  const quint64 total = now.total - before.total;
  const quint64 active = now.active - before.active;
  if (active > total) {
    return std::nullopt;
  }
  return static_cast<double>(active) / static_cast<double>(total);
}

void parseMemory(const QByteArray& bytes, protocol::SystemMetrics::Memory& memory, QStringList& diagnostics) {
  QHash<QByteArray, quint64> values;
  for (const QByteArray& line : bytes.split('\n')) {
    const qsizetype colon = line.indexOf(':');
    if (colon < 0) {
      continue;
    }
    const QList<QByteArray> fields = line.mid(colon + 1).simplified().split(' ');
    if (fields.size() != 2 || fields[1] != "kB") {
      continue;
    }
    const auto kib = unsignedValue(fields[0]);
    if (kib && *kib <= std::numeric_limits<quint64>::max() / kKib) {
      values.insert(line.left(colon), *kib * kKib);
    }
  }
  memory.total_bytes =
      values.contains("MemTotal") && values["MemTotal"] > 0 ? std::optional(values["MemTotal"]) : std::nullopt;
  memory.available_bytes = values.contains("MemAvailable") ? std::optional(values["MemAvailable"]) : std::nullopt;
  memory.swap_total_bytes = values.contains("SwapTotal") ? std::optional(values["SwapTotal"]) : std::nullopt;
  memory.swap_available_bytes = values.contains("SwapFree") ? std::optional(values["SwapFree"]) : std::nullopt;
  if (memory.total_bytes && memory.available_bytes && *memory.available_bytes > *memory.total_bytes) {
    memory.available_bytes.reset();
    diagnostics.append(QStringLiteral("Physical memory byte relationship is invalid"));
  }
  if (memory.swap_total_bytes && memory.swap_available_bytes &&
      *memory.swap_available_bytes > *memory.swap_total_bytes) {
    memory.swap_available_bytes.reset();
    diagnostics.append(QStringLiteral("Swap byte relationship is invalid"));
  }
}

}  // namespace

LinuxSysMetricsCollector::LinuxSysMetricsCollector(std::shared_ptr<const LinuxSysMetricsAccess> access)
    : access_(access ? std::move(access) : std::make_shared<NativeAccess>()) {}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
SysMetricsCollectionResult LinuxSysMetricsCollector::collect() {
  protocol::SystemMetrics metrics;
  QStringList diagnostics;
  const qint64 now_ms = access_->monotonicMilliseconds();

  if (const auto stat = access_->readFile(QStringLiteral("/proc/stat"), kProcLimit)) {
    const auto counters = parseCpu(*stat, diagnostics);
    for (auto iterator = counters.cbegin(); iterator != counters.cend(); ++iterator) {
      std::optional<double> ratio;
      if (cpu_counters_.contains(iterator.key())) {
        ratio = usage(iterator.value(), cpu_counters_[iterator.key()], diagnostics);
      }
      if (iterator.key() == QStringLiteral("cpu")) {
        metrics.cpu.usage_ratio = ratio;
      } else {
        protocol::SystemMetrics::LogicalCpu cpu{.name = iterator.key(), .usage_ratio = ratio};
        const QString index = iterator.key().sliced(3);
        if (const auto khz = access_->readFile(
                QStringLiteral("/sys/devices/system/cpu/cpu%1/cpufreq/scaling_cur_freq").arg(index), kSysfsLimit)) {
          const auto value = unsignedValue(*khz);
          if (value && *value <= std::numeric_limits<quint64>::max() / 1000) {
            cpu.frequency_hz = *value * 1000;
          }
        }
        metrics.cpu.logical_cpus.append(std::move(cpu));
      }
    }
    cpu_counters_ = counters;
  } else {
    diagnostics.append(QStringLiteral("/proc/stat is unavailable"));
  }

  if (const auto meminfo = access_->readFile(QStringLiteral("/proc/meminfo"), kProcLimit)) {
    parseMemory(*meminfo, metrics.memory, diagnostics);
  } else {
    diagnostics.append(QStringLiteral("/proc/meminfo is unavailable"));
  }

  if (const auto uptime = access_->readFile(QStringLiteral("/proc/uptime"), kSysfsLimit)) {
    metrics.system.uptime_seconds = nonnegativeDouble(uptime->split(' ').value(0));
  }
  if (const auto load = access_->readFile(QStringLiteral("/proc/loadavg"), kSysfsLimit)) {
    const auto fields = load->simplified().split(' ');
    if (fields.size() >= 3) {
      metrics.system.load_average_1m = nonnegativeDouble(fields[0]);
      metrics.system.load_average_5m = nonnegativeDouble(fields[1]);
      metrics.system.load_average_15m = nonnegativeDouble(fields[2]);
    }
  }

  for (const auto& source : access_->storageVolumes()) {
    if (!source.ready || source.total_bytes == 0 || source.available_bytes > source.total_bytes) {
      continue;
    }
    metrics.storage_volumes.append({.mount_point = source.mount_point,
                                    .device_name = source.device_name,
                                    .primary = source.mount_point == QStringLiteral("/"),
                                    .read_only = source.read_only,
                                    .total_bytes = source.total_bytes,
                                    .available_bytes = source.available_bytes});
  }

  for (const QString& zone : access_->directoryEntries(QStringLiteral("/sys/class/thermal"))) {
    if (!zone.startsWith(QStringLiteral("thermal_zone"))) {
      continue;
    }
    const QString base = QStringLiteral("/sys/class/thermal/%1/").arg(zone);
    const auto type = access_->readFile(base + QStringLiteral("type"), kSysfsLimit);
    if (!type) {
      continue;
    }
    const QByteArray normalized = type->trimmed().toLower();
    if (normalized != "cpu-thermal" && normalized != "cpu_thermal" && normalized != "soc_thermal") {
      continue;
    }
    if (const auto raw = access_->readFile(base + QStringLiteral("temp"), kSysfsLimit)) {
      const auto milli = nonnegativeDouble(*raw);
      if (milli) {
        metrics.cpu.temperature_celsius = *milli / 1000.0;
      }
    }
    break;
  }

  const double elapsed = previous_time_ms_ >= 0 && now_ms > previous_time_ms_
                             ? static_cast<double>(now_ms - previous_time_ms_) / 1000.0
                             : 0.0;
  QHash<QString, QPair<quint64, quint64>> next_network;
  for (const QString& name : access_->directoryEntries(QStringLiteral("/sys/class/net"))) {
    if (name == QStringLiteral("lo")) {
      continue;
    }
    const QString base = QStringLiteral("/sys/class/net/%1/statistics/").arg(name);
    const auto rx_file = access_->readFile(base + QStringLiteral("rx_bytes"), kSysfsLimit);
    const auto tx_file = access_->readFile(base + QStringLiteral("tx_bytes"), kSysfsLimit);
    const auto received = rx_file ? unsignedValue(*rx_file) : std::nullopt;
    const auto transmitted = tx_file ? unsignedValue(*tx_file) : std::nullopt;
    if (!received && !transmitted) {
      continue;
    }
    protocol::SystemMetrics::NetworkInterface network{.name = name, .rx_bytes = received, .tx_bytes = transmitted};
    if (received && transmitted) {
      next_network.insert(name, {*received, *transmitted});
      if (elapsed > 0.0 && network_counters_.contains(name)) {
        const auto previous = network_counters_[name];
        if (*received >= previous.first) {
          network.rx_bytes_per_second = static_cast<double>(*received - previous.first) / elapsed;
        } else {
          diagnostics.append(QStringLiteral("Network RX counter reset for %1").arg(name));
        }
        if (*transmitted >= previous.second) {
          network.tx_bytes_per_second = static_cast<double>(*transmitted - previous.second) / elapsed;
        } else {
          diagnostics.append(QStringLiteral("Network TX counter reset for %1").arg(name));
        }
      }
    }
    metrics.network_interfaces.append(std::move(network));
  }
  network_counters_ = std::move(next_network);
  previous_time_ms_ = now_ms;

  const auto compatible = access_->readFile(QStringLiteral("/sys/firmware/devicetree/base/compatible"), kSysfsLimit);
  if (compatible && compatible->contains("raspberrypi,")) {
    diagnostics.append(QStringLiteral("Explicit Raspberry Pi GPU metrics are unavailable"));
  } else {
    diagnostics.append(QStringLiteral("Raspberry Pi GPU enrichment is unavailable on this Linux host"));
  }

  if (!metrics.hasAllBaselineFields()) {
    diagnostics.append(QStringLiteral("One or more baseline system metrics are unavailable"));
  }
  return {.metrics = std::move(metrics), .diagnostics = std::move(diagnostics)};
}

}  // namespace dashboard::sysmetrics
