#include "weather/weather_models.h"

#include <QLocale>

#include <algorithm>

namespace dashboard::weather {
// NOLINTBEGIN(cppcoreguidelines-narrowing-conversions,readability-braces-around-statements,readability-inconsistent-declaration-parameter-name)
namespace {
QDateTime localTime(const QDateTime& utc, int offset) { return utc.addSecs(offset); }
}  // namespace

HourlyForecastModel::HourlyForecastModel(QObject* parent) : QAbstractListModel(parent) {}
int HourlyForecastModel::rowCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : rows_.size(); }
QVariant HourlyForecastModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= rows_.size()) return {};
  const auto& row = rows_.at(index.row());
  switch (role) {
    case TimestampRole:
      return row.timestampUtc;
    case LocalHourRole:
      return QStringLiteral("%1").arg(localTime(row.timestampUtc, timezoneOffsetSeconds_).time().hour(), 2, 10,
                                      QLatin1Char('0'));
    case IconCodeRole:
      return row.iconCode;
    case TemperatureRole:
      return row.temperatureCelsius;
    case PrecipitationRole:
      return row.precipitationProbabilityPercent;
    case TrendPositionRole:
      return maximumTemperature_ == minimumTemperature_
                 ? 0.5
                 : (row.temperatureCelsius - minimumTemperature_) / (maximumTemperature_ - minimumTemperature_);
    case PreviousTrendPositionRole: {
      if (index.row() == 0) return 0.5;
      const double previous = rows_.at(index.row() - 1).temperatureCelsius;
      return maximumTemperature_ == minimumTemperature_
                 ? 0.5
                 : (previous - minimumTemperature_) / (maximumTemperature_ - minimumTemperature_);
    }
    default:
      return {};
  }
}
QHash<int, QByteArray> HourlyForecastModel::roleNames() const {
  return {{TimestampRole, "timestampUtc"},
          {LocalHourRole, "localHour"},
          {IconCodeRole, "iconCode"},
          {TemperatureRole, "temperatureCelsius"},
          {PrecipitationRole, "precipitationProbabilityPercent"},
          {TrendPositionRole, "trendPosition"},
          {PreviousTrendPositionRole, "previousTrendPosition"}};
}
void HourlyForecastModel::replace(const QVector<HourlyForecast>& rows, int timezoneOffsetSeconds) {
  beginResetModel();
  rows_ = rows.mid(0, 8);
  timezoneOffsetSeconds_ = timezoneOffsetSeconds;
  if (!rows_.isEmpty()) {
    const auto extrema = std::ranges::minmax_element(rows_, {}, &HourlyForecast::temperatureCelsius);
    minimumTemperature_ = extrema.min->temperatureCelsius;
    maximumTemperature_ = extrema.max->temperatureCelsius;
  }
  endResetModel();
}

DailyForecastModel::DailyForecastModel(QObject* parent) : QAbstractListModel(parent) {}
int DailyForecastModel::rowCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : rows_.size(); }
QVariant DailyForecastModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= rows_.size()) return {};
  const auto& row = rows_.at(index.row());
  switch (role) {
    case TimestampRole:
      return row.timestampUtc;
    case WeekdayRole:
      return QLocale().dayName(localTime(row.timestampUtc, timezoneOffsetSeconds_).date().dayOfWeek(),
                               QLocale::ShortFormat);
    case IconCodeRole:
      return row.iconCode;
    case MinimumRole:
      return row.minimumCelsius;
    case MaximumRole:
      return row.maximumCelsius;
    case AverageRole:
      return row.averageCelsius ? QVariant::fromValue(*row.averageCelsius) : QVariant{};
    case PrecipitationRole:
      return row.precipitationProbabilityPercent;
    default:
      return {};
  }
}
QHash<int, QByteArray> DailyForecastModel::roleNames() const {
  return {{TimestampRole, "timestampUtc"},
          {WeekdayRole, "weekday"},
          {IconCodeRole, "iconCode"},
          {MinimumRole, "minimumCelsius"},
          {MaximumRole, "maximumCelsius"},
          {AverageRole, "averageCelsius"},
          {PrecipitationRole, "precipitationProbabilityPercent"}};
}
void DailyForecastModel::replace(const QVector<DailyForecast>& rows, int timezoneOffsetSeconds) {
  beginResetModel();
  rows_ = rows.mid(0, 5);
  timezoneOffsetSeconds_ = timezoneOffsetSeconds;
  endResetModel();
}
// NOLINTEND(cppcoreguidelines-narrowing-conversions,readability-braces-around-statements,readability-inconsistent-declaration-parameter-name)
}  // namespace dashboard::weather
