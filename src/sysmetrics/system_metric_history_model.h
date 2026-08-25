#pragma once

#include <QAbstractListModel>

#include <cstdint>
#include <optional>

namespace dashboard::sysmetrics {

class SystemMetricHistoryModel final : public QAbstractListModel {
  Q_OBJECT

 public:
  enum class Role : std::uint16_t { ElapsedMilliseconds = Qt::UserRole + 1, CpuUsageRatio, MemoryUsageRatio };

  explicit SystemMetricHistoryModel(QObject* parent = nullptr);

  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  void appendSample(qint64 elapsed_milliseconds, std::optional<double> cpu_usage_ratio,
                    std::optional<double> memory_usage_ratio);

 private:
  struct Sample {
    qint64 elapsed_milliseconds;
    std::optional<double> cpu_usage_ratio;
    std::optional<double> memory_usage_ratio;
  };

  QList<Sample> samples_;
};

}  // namespace dashboard::sysmetrics
