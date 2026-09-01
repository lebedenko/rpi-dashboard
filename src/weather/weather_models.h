#pragma once

#include "weather/weather_types.h"

#include <QAbstractListModel>

namespace dashboard::weather {
// Qt item roles intentionally use the integer enum convention required by QAbstractItemModel.
// NOLINTBEGIN(cppcoreguidelines-use-enum-class,performance-enum-size)

class HourlyForecastModel final : public QAbstractListModel {
  Q_OBJECT
 public:
  enum Role {
    TimestampRole = Qt::UserRole + 1,
    LocalHourRole,
    IconCodeRole,
    TemperatureRole,
    PrecipitationRole,
    TrendPositionRole,
    PreviousTrendPositionRole
  };
  explicit HourlyForecastModel(QObject* parent = nullptr);
  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
  void replace(const QVector<HourlyForecast>& rows, int timezoneOffsetSeconds);

 private:
  QVector<HourlyForecast> rows_;
  int timezoneOffsetSeconds_{};
  double minimumTemperature_{};
  double maximumTemperature_{};
};

class DailyForecastModel final : public QAbstractListModel {
  Q_OBJECT
 public:
  enum Role {
    TimestampRole = Qt::UserRole + 1,
    WeekdayRole,
    IconCodeRole,
    MinimumRole,
    MaximumRole,
    AverageRole,
    PrecipitationRole
  };
  explicit DailyForecastModel(QObject* parent = nullptr);
  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
  void replace(const QVector<DailyForecast>& rows, int timezoneOffsetSeconds);

 private:
  QVector<DailyForecast> rows_;
  int timezoneOffsetSeconds_{};
};

// NOLINTEND(cppcoreguidelines-use-enum-class,performance-enum-size)
}  // namespace dashboard::weather
