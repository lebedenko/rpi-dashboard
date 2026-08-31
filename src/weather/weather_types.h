#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>

#include <optional>

namespace dashboard::weather {
// NOLINTBEGIN(readability-identifier-naming)

struct Location {
  QString city;
  QString country;
  double latitude{};
  double longitude{};

  [[nodiscard]] QString cacheKey() const;
};

struct CurrentConditions {
  QDateTime observedUtc;
  QString condition;
  QString iconCode;
  double temperatureCelsius{};
  double feelsLikeCelsius{};
  double highCelsius{};
  double lowCelsius{};
  double humidityPercent{};
  double pressureHpa{};
  double windSpeedKmh{};
  double windDegrees{};
  std::optional<double> rainMillimetres;
};

struct HourlyForecast {
  QDateTime timestampUtc;
  QString iconCode;
  double temperatureCelsius{};
  double precipitationProbabilityPercent{};
  std::optional<double> rainMillimetres;
};

struct DailyForecast {
  QDateTime timestampUtc;
  QDateTime sunriseUtc;
  QDateTime sunsetUtc;
  QString iconCode;
  double minimumCelsius{};
  double maximumCelsius{};
  double precipitationProbabilityPercent{};
};

struct AirQuality {
  int index{};
  QString category;
  std::optional<double> pm25MicrogramsPerCubicMetre;
};

struct Snapshot {
  QString provider;
  Location location;
  QString timezone;
  int timezoneOffsetSeconds{};
  QDateTime fetchedUtc;
  CurrentConditions current;
  QVector<HourlyForecast> hourly;
  QVector<DailyForecast> daily;
  std::optional<AirQuality> airQuality;
  QDateTime sunsetUtc;
  double todayRainProbabilityPercent{};
};

// NOLINTEND(readability-identifier-naming)
}  // namespace dashboard::weather
