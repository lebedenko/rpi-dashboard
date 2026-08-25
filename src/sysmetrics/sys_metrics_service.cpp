#include "sysmetrics/sys_metrics_service.h"

#include <QtConcurrentRun>

#include <cmath>
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
  Q_ASSERT(collector_);
  timer_.setInterval(qMax(1, sampling_interval_ms));
  connect(&timer_, &QTimer::timeout, this, &SysMetricsService::refresh);
  connect(&watcher_, &QFutureWatcherBase::finished, this, &SysMetricsService::collectionFinished);
  timer_.start();
  QMetaObject::invokeMethod(this, &SysMetricsService::refresh, Qt::QueuedConnection);
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
QVariant SysMetricsService::memoryUsageRatio() const {
  if (!current_metrics_.memory.total_bytes || !current_metrics_.memory.available_bytes ||
      *current_metrics_.memory.total_bytes == 0 ||
      *current_metrics_.memory.available_bytes > *current_metrics_.memory.total_bytes) {
    return {};
  }
  return 1.0 - (static_cast<double>(*current_metrics_.memory.available_bytes) /
                static_cast<double>(*current_metrics_.memory.total_bytes));
}
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
  SysMetricsCollectionResult result = watcher_.result();
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
    emit currentMetricsChanged();
    last_success_utc_ = QDateTime::currentDateTimeUtc();
    emit lastSuccessUtcChanged();
    setState(current_metrics_.hasAllBaselineFields() ? State::Ready : State::Partial);
  }
  if (std::exchange(refresh_pending_, false)) {
    QMetaObject::invokeMethod(this, &SysMetricsService::refresh, Qt::QueuedConnection);
  }
}
void SysMetricsService::setState(State state) {
  if (state_ != state) {
    state_ = state;
    emit stateChanged();
  }
}

}  // namespace dashboard::sysmetrics
