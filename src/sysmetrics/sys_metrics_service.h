#pragma once

#include "sysmetrics/sys_metrics_collector.h"
#include "sysmetrics/system_metric_history_model.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QObject>
#include <QTimer>
#include <QVariant>

#include <cstdint>
#include <memory>

namespace dashboard::sysmetrics {

class SysMetricsService final : public QObject {
  Q_OBJECT
  Q_PROPERTY(State state READ state NOTIFY stateChanged)
  Q_PROPERTY(dashboard::protocol::SystemMetrics currentMetrics READ currentMetrics NOTIFY currentMetricsChanged)
  Q_PROPERTY(QDateTime lastSuccessUtc READ lastSuccessUtc NOTIFY lastSuccessUtcChanged)
  Q_PROPERTY(QStringList diagnostics READ diagnostics NOTIFY diagnosticsChanged)
  Q_PROPERTY(QVariant cpuUsageRatio READ cpuUsageRatio NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant memoryUsageRatio READ memoryUsageRatio NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant cpuTemperatureCelsius READ cpuTemperatureCelsius NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant uptimeSeconds READ uptimeSeconds NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant averageCpuFrequencyHz READ averageCpuFrequencyHz NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant memoryTotalBytes READ memoryTotalBytes NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant memoryAvailableBytes READ memoryAvailableBytes NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant memoryUsedBytes READ memoryUsedBytes NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant swapTotalBytes READ swapTotalBytes NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant swapAvailableBytes READ swapAvailableBytes NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant swapUsedBytes READ swapUsedBytes NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant primaryStorageTotalBytes READ primaryStorageTotalBytes NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant primaryStorageUsedBytes READ primaryStorageUsedBytes NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant primaryStorageUsageRatio READ primaryStorageUsageRatio NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant gpuName READ gpuName NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant gpuUsageRatio READ gpuUsageRatio NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant gpuMemoryTotalBytes READ gpuMemoryTotalBytes NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant gpuMemoryUsedBytes READ gpuMemoryUsedBytes NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant gpuCoreClockHz READ gpuCoreClockHz NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant gpuMemoryClockHz READ gpuMemoryClockHz NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant gpuTemperatureCelsius READ gpuTemperatureCelsius NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant networkReceiveBytesPerSecond READ networkReceiveBytesPerSecond NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant networkTransmitBytesPerSecond READ networkTransmitBytesPerSecond NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant networkInterfaceName READ networkInterfaceName NOTIFY currentMetricsChanged)
  Q_PROPERTY(QVariant bootTimeUtc READ bootTimeUtc NOTIFY currentMetricsChanged)
  Q_PROPERTY(QAbstractItemModel* usageHistoryModel READ usageHistoryModel CONSTANT)

 public:
  enum class State : std::uint8_t { Idle, Collecting, Ready, Partial, Error };
  Q_ENUM(State)

  explicit SysMetricsService(std::shared_ptr<SysMetricsCollector> collector, int sampling_interval_ms = 1000,
                             QObject* parent = nullptr);
  [[nodiscard]] State state() const;
  [[nodiscard]] protocol::SystemMetrics currentMetrics() const;
  [[nodiscard]] QDateTime lastSuccessUtc() const;
  [[nodiscard]] QStringList diagnostics() const;
  [[nodiscard]] QVariant cpuUsageRatio() const;
  [[nodiscard]] QVariant memoryUsageRatio() const;
  [[nodiscard]] QVariant cpuTemperatureCelsius() const;
  [[nodiscard]] QVariant uptimeSeconds() const;
  [[nodiscard]] QVariant averageCpuFrequencyHz() const;
  [[nodiscard]] QVariant memoryTotalBytes() const;
  [[nodiscard]] QVariant memoryAvailableBytes() const;
  [[nodiscard]] QVariant memoryUsedBytes() const;
  [[nodiscard]] QVariant swapTotalBytes() const;
  [[nodiscard]] QVariant swapAvailableBytes() const;
  [[nodiscard]] QVariant swapUsedBytes() const;
  [[nodiscard]] QVariant primaryStorageTotalBytes() const;
  [[nodiscard]] QVariant primaryStorageUsedBytes() const;
  [[nodiscard]] QVariant primaryStorageUsageRatio() const;
  [[nodiscard]] QVariant gpuName() const;
  [[nodiscard]] QVariant gpuUsageRatio() const;
  [[nodiscard]] QVariant gpuMemoryTotalBytes() const;
  [[nodiscard]] QVariant gpuMemoryUsedBytes() const;
  [[nodiscard]] QVariant gpuCoreClockHz() const;
  [[nodiscard]] QVariant gpuMemoryClockHz() const;
  [[nodiscard]] QVariant gpuTemperatureCelsius() const;
  [[nodiscard]] QVariant networkReceiveBytesPerSecond() const;
  [[nodiscard]] QVariant networkTransmitBytesPerSecond() const;
  [[nodiscard]] QVariant networkInterfaceName() const;
  [[nodiscard]] QVariant bootTimeUtc() const;
  [[nodiscard]] QAbstractItemModel* usageHistoryModel();

 public slots:
  void refresh();

 signals:
  void stateChanged();
  void currentMetricsChanged();
  void lastSuccessUtcChanged();
  void diagnosticsChanged();

 private:
  void collectionFinished();
  void setState(State state);
  std::shared_ptr<SysMetricsCollector> collector_;
  QFutureWatcher<SysMetricsCollectionResult> watcher_;
  QTimer timer_;
  bool refresh_pending_{false};
  State state_{State::Idle};
  protocol::SystemMetrics current_metrics_;
  QDateTime last_success_utc_;
  QStringList diagnostics_;
  QElapsedTimer elapsed_timer_;
  SystemMetricHistoryModel usage_history_model_;
};

}  // namespace dashboard::sysmetrics
