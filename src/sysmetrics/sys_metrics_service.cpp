#include "sysmetrics/sys_metrics_service.h"

#include <QtConcurrentRun>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace dashboard::sysmetrics {
namespace {
template <typename T>
QVariant optionalVariant(const std::optional<T>& value) {
  return value ? QVariant::fromValue(*value) : QVariant{};
}
}  // namespace

SysMetricsService::SysMetricsService(std::shared_ptr<SysMetricsCollector> collector, int sampling_interval_ms,
                                     QObject* parent)
    : QObject(parent), collector_(std::move(collector)) {
  if (!collector_) {
    throw std::invalid_argument("SysMetricsService requires a collector");
  }
  elapsed_timer_.start();
  timer_.setInterval(qMax(1, sampling_interval_ms));
  connect(&timer_, &QTimer::timeout, this, &SysMetricsService::refresh);
  connect(&watcher_, &QFutureWatcherBase::finished, this, &SysMetricsService::collectionFinished);
  timer_.start();
  QMetaObject::invokeMethod(this, "refresh", Qt::QueuedConnection);
}
SysMetricsService::State SysMetricsService::state() const { return state_; }
protocol::SystemMetrics SysMetricsService::currentMetrics() const { return current_metrics_; }
QDateTime SysMetricsService::lastSuccessUtc() const { return last_success_utc_; }
QStringList SysMetricsService::diagnostics() const { return diagnostics_; }
QVariant SysMetricsService::cpuUsageRatio() const { return optionalVariant(current_metrics_.cpu.usage_ratio); }
QVariant SysMetricsService::cpuTemperatureCelsius() const {
  return optionalVariant(current_metrics_.cpu.temperature_celsius);
}
QVariant SysMetricsService::uptimeSeconds() const { return optionalVariant(current_metrics_.system.uptime_seconds); }
QVariant SysMetricsService::averageCpuFrequencyHz() const {
  quint64 sum = 0;
  quint64 count = 0;
  for (const auto& cpu : current_metrics_.cpu.logical_cpus) {
    if (!cpu.frequency_hz || *cpu.frequency_hz > std::numeric_limits<quint64>::max() - sum) {
      continue;
    }
    sum += *cpu.frequency_hz;
    ++count;
  }
  return count > 0 ? QVariant::fromValue(static_cast<double>(sum) / static_cast<double>(count)) : QVariant{};
}
QVariant SysMetricsService::memoryTotalBytes() const { return optionalVariant(current_metrics_.memory.total_bytes); }
QVariant SysMetricsService::memoryAvailableBytes() const {
  return optionalVariant(current_metrics_.memory.available_bytes);
}
QVariant SysMetricsService::memoryUsedBytes() const {
  const auto& memory = current_metrics_.memory;
  if (!memory.total_bytes || !memory.available_bytes || *memory.available_bytes > *memory.total_bytes) {
    return {};
  }
  return QVariant::fromValue(*memory.total_bytes - *memory.available_bytes);
}
QVariant SysMetricsService::swapTotalBytes() const { return optionalVariant(current_metrics_.memory.swap_total_bytes); }
QVariant SysMetricsService::swapAvailableBytes() const {
  return optionalVariant(current_metrics_.memory.swap_available_bytes);
}
QVariant SysMetricsService::swapUsedBytes() const {
  const auto& memory = current_metrics_.memory;
  if (!memory.swap_total_bytes || !memory.swap_available_bytes ||
      *memory.swap_available_bytes > *memory.swap_total_bytes) {
    return {};
  }
  return QVariant::fromValue(*memory.swap_total_bytes - *memory.swap_available_bytes);
}
QVariant SysMetricsService::primaryStorageTotalBytes() const {
  const auto found =
      std::ranges::find_if(current_metrics_.storage_volumes, [](const auto& volume) { return volume.primary; });
  return found == current_metrics_.storage_volumes.cend() ? QVariant{} : optionalVariant(found->total_bytes);
}
QVariant SysMetricsService::primaryStorageUsedBytes() const {
  const auto found =
      std::ranges::find_if(current_metrics_.storage_volumes, [](const auto& volume) { return volume.primary; });
  if (found == current_metrics_.storage_volumes.cend() || !found->total_bytes || !found->available_bytes ||
      *found->available_bytes > *found->total_bytes) {
    return {};
  }
  return QVariant::fromValue(*found->total_bytes - *found->available_bytes);
}
QVariant SysMetricsService::primaryStorageUsageRatio() const {
  const auto found =
      std::ranges::find_if(current_metrics_.storage_volumes, [](const auto& volume) { return volume.primary; });
  if (found == current_metrics_.storage_volumes.cend() || !found->total_bytes || !found->available_bytes ||
      *found->total_bytes == 0 || *found->available_bytes > *found->total_bytes) {
    return {};
  }
  return QVariant::fromValue(static_cast<double>(*found->total_bytes - *found->available_bytes) /
                             static_cast<double>(*found->total_bytes));
}
QVariant SysMetricsService::gpuName() const {
  return current_metrics_.gpus.isEmpty() || current_metrics_.gpus.first().name.isEmpty()
             ? QVariant{}
             : QVariant::fromValue(current_metrics_.gpus.first().name);
}
QVariant SysMetricsService::gpuUsageRatio() const {
  return current_metrics_.gpus.isEmpty() ? QVariant{} : optionalVariant(current_metrics_.gpus.first().usage_ratio);
}
QVariant SysMetricsService::gpuMemoryTotalBytes() const {
  return current_metrics_.gpus.isEmpty() ? QVariant{}
                                         : optionalVariant(current_metrics_.gpus.first().memory_total_bytes);
}
QVariant SysMetricsService::gpuMemoryUsedBytes() const {
  return current_metrics_.gpus.isEmpty() ? QVariant{}
                                         : optionalVariant(current_metrics_.gpus.first().memory_used_bytes);
}
QVariant SysMetricsService::gpuCoreClockHz() const {
  return current_metrics_.gpus.isEmpty() ? QVariant{} : optionalVariant(current_metrics_.gpus.first().core_clock_hz);
}
QVariant SysMetricsService::gpuMemoryClockHz() const {
  return current_metrics_.gpus.isEmpty() ? QVariant{} : optionalVariant(current_metrics_.gpus.first().memory_clock_hz);
}
QVariant SysMetricsService::gpuTemperatureCelsius() const {
  return current_metrics_.gpus.isEmpty() ? QVariant{}
                                         : optionalVariant(current_metrics_.gpus.first().temperature_celsius);
}
QVariant SysMetricsService::networkReceiveBytesPerSecond() const {
  double total = 0.0;
  bool available = false;
  for (const auto& interface : current_metrics_.network_interfaces) {
    if (interface.name == QStringLiteral("lo") || !interface.rx_bytes_per_second) {
      continue;
    }
    total += *interface.rx_bytes_per_second;
    available = true;
  }
  return available ? QVariant(total) : QVariant{};
}
QVariant SysMetricsService::networkTransmitBytesPerSecond() const {
  double total = 0.0;
  bool available = false;
  for (const auto& interface : current_metrics_.network_interfaces) {
    if (interface.name == QStringLiteral("lo") || !interface.tx_bytes_per_second) {
      continue;
    }
    total += *interface.tx_bytes_per_second;
    available = true;
  }
  return available ? QVariant(total) : QVariant{};
}
QVariant SysMetricsService::networkInterfaceName() const {
  QString name;
  for (const auto& interface : current_metrics_.network_interfaces) {
    if (interface.name == QStringLiteral("lo")) {
      continue;
    }
    if (!name.isEmpty()) {
      return {};
    }
    name = interface.name;
  }
  return name.isEmpty() ? QVariant{} : QVariant(name);
}
QVariant SysMetricsService::bootTimeUtc() const {
  if (!current_metrics_.system.uptime_seconds || !last_success_utc_.isValid()) {
    return {};
  }
  return last_success_utc_.addMSecs(-qRound64(*current_metrics_.system.uptime_seconds * 1000.0));
}
QVariant SysMetricsService::memoryUsageRatio() const {
  if (!current_metrics_.memory.total_bytes || !current_metrics_.memory.available_bytes ||
      *current_metrics_.memory.total_bytes == 0 ||
      *current_metrics_.memory.available_bytes > *current_metrics_.memory.total_bytes) {
    return {};
  }
  return 1.0 - (static_cast<double>(*current_metrics_.memory.available_bytes) /
                static_cast<double>(*current_metrics_.memory.total_bytes));
}
QAbstractItemModel* SysMetricsService::usageHistoryModel() { return &usage_history_model_; }
void SysMetricsService::refresh() {
  if (state_ == State::Collecting) {
    refresh_pending_ = true;
    return;
  }
  setState(State::Collecting);
  const auto collector = collector_;
  watcher_.setFuture(QtConcurrent::run([collector] { return collector->collect(); }));
}
void SysMetricsService::collectionFinished() {
  SysMetricsCollectionResult result;
  try {
    result = watcher_.result();
  } catch (const std::exception& exception) {
    diagnostics_ = {QStringLiteral("System metrics collection failed: %1").arg(exception.what())};
    emit diagnosticsChanged();
    setState(State::Error);
    if (std::exchange(refresh_pending_, false)) {
      QMetaObject::invokeMethod(this, "refresh", Qt::QueuedConnection);
    }
    return;
  } catch (...) {
    diagnostics_ = {QStringLiteral("System metrics collection failed with an unknown error")};
    emit diagnosticsChanged();
    setState(State::Error);
    if (std::exchange(refresh_pending_, false)) {
      QMetaObject::invokeMethod(this, "refresh", Qt::QueuedConnection);
    }
    return;
  }
  const auto cpu_usage_ratio = result.metrics.cpu.usage_ratio;
  std::optional<double> memory_usage_ratio;
  if (result.metrics.memory.total_bytes && result.metrics.memory.available_bytes &&
      *result.metrics.memory.total_bytes > 0 &&
      *result.metrics.memory.available_bytes <= *result.metrics.memory.total_bytes) {
    memory_usage_ratio = 1.0 - (static_cast<double>(*result.metrics.memory.available_bytes) /
                                static_cast<double>(*result.metrics.memory.total_bytes));
  }
  usage_history_model_.appendSample(elapsed_timer_.elapsed(), cpu_usage_ratio, memory_usage_ratio);
  if (diagnostics_ != result.diagnostics) {
    diagnostics_ = std::move(result.diagnostics);
    emit diagnosticsChanged();
  }
  if (!result.metrics.hasAnyValue()) {
    if (diagnostics_.isEmpty()) {
      diagnostics_.append(QStringLiteral("System metrics collection produced no usable values"));
      emit diagnosticsChanged();
    }
    setState(State::Error);
  } else {
    current_metrics_ = std::move(result.metrics);
    last_success_utc_ = QDateTime::currentDateTimeUtc();
    emit currentMetricsChanged();
    emit lastSuccessUtcChanged();
    setState(current_metrics_.hasAllBaselineFields() ? State::Ready : State::Partial);
  }
  if (std::exchange(refresh_pending_, false)) {
    QMetaObject::invokeMethod(this, "refresh", Qt::QueuedConnection);
  }
}
void SysMetricsService::setState(State state) {
  if (state_ != state) {
    state_ = state;
    emit stateChanged();
  }
}

}  // namespace dashboard::sysmetrics
