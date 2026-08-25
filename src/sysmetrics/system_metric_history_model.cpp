#include "sysmetrics/system_metric_history_model.h"

#include <QVariant>

namespace dashboard::sysmetrics {
namespace {
constexpr qint64 kHistoryWindowMilliseconds = 60'000;

QVariant optionalRatio(const std::optional<double>& value) { return value ? QVariant::fromValue(*value) : QVariant{}; }
}  // namespace

SystemMetricHistoryModel::SystemMetricHistoryModel(QObject* parent) : QAbstractListModel(parent) {}

int SystemMetricHistoryModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(samples_.size());
}

QVariant SystemMetricHistoryModel::data(const QModelIndex& index, int role) const {
  if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
    return {};
  }
  const Sample& sample = samples_.at(index.row());
  switch (role) {
    case static_cast<int>(Role::ElapsedMilliseconds):
      return sample.elapsed_milliseconds;
    case static_cast<int>(Role::CpuUsageRatio):
      return optionalRatio(sample.cpu_usage_ratio);
    case static_cast<int>(Role::MemoryUsageRatio):
      return optionalRatio(sample.memory_usage_ratio);
    default:
      return {};
  }
}

QHash<int, QByteArray> SystemMetricHistoryModel::roleNames() const {
  return {{static_cast<int>(Role::ElapsedMilliseconds), QByteArrayLiteral("elapsedMilliseconds")},
          {static_cast<int>(Role::CpuUsageRatio), QByteArrayLiteral("cpuUsageRatio")},
          {static_cast<int>(Role::MemoryUsageRatio), QByteArrayLiteral("memoryUsageRatio")}};
}

void SystemMetricHistoryModel::appendSample(qint64 elapsed_milliseconds, std::optional<double> cpu_usage_ratio,
                                            std::optional<double> memory_usage_ratio) {
  const qint64 cutoff = elapsed_milliseconds - kHistoryWindowMilliseconds;
  qsizetype remove_count = 0;
  while (remove_count < samples_.size() && samples_.at(remove_count).elapsed_milliseconds < cutoff) {
    ++remove_count;
  }
  if (remove_count > 0) {
    beginRemoveRows({}, 0, static_cast<int>(remove_count - 1));
    samples_.remove(0, remove_count);
    endRemoveRows();
  }

  const int row = static_cast<int>(samples_.size());
  beginInsertRows({}, row, row);
  samples_.append({.elapsed_milliseconds = elapsed_milliseconds,
                   .cpu_usage_ratio = cpu_usage_ratio,
                   .memory_usage_ratio = memory_usage_ratio});
  endInsertRows();
}

}  // namespace dashboard::sysmetrics
