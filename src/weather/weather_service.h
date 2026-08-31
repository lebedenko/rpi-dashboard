#pragma once

#include "weather/weather_config.h"
#include "weather/weather_models.h"
#include "weather/weather_provider.h"

#include <QObject>
#include <QTimer>

#include <cstdint>
#include <memory>

namespace dashboard::weather {

class WeatherService final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString state READ state NOTIFY changed)
  Q_PROPERTY(bool stale READ stale NOTIFY changed)
  Q_PROPERTY(QDateTime lastSuccessUtc READ lastSuccessUtc NOTIFY changed)
  Q_PROPERTY(QString diagnostics READ diagnostics NOTIFY changed)
  Q_PROPERTY(QString city READ city NOTIFY changed)
  Q_PROPERTY(QString country READ country NOTIFY changed)
  Q_PROPERTY(QString condition READ condition NOTIFY changed)
  Q_PROPERTY(QString iconCode READ iconCode NOTIFY changed)
  Q_PROPERTY(double temperatureCelsius READ temperatureCelsius NOTIFY changed)
  Q_PROPERTY(double feelsLikeCelsius READ feelsLikeCelsius NOTIFY changed)
  Q_PROPERTY(double highCelsius READ highCelsius NOTIFY changed)
  Q_PROPERTY(double lowCelsius READ lowCelsius NOTIFY changed)
  Q_PROPERTY(double humidityPercent READ humidityPercent NOTIFY changed)
  Q_PROPERTY(double windSpeedKmh READ windSpeedKmh NOTIFY changed)
  Q_PROPERTY(QString windDirection READ windDirection NOTIFY changed)
  Q_PROPERTY(QAbstractItemModel* hourlyModel READ hourlyModel CONSTANT)
  Q_PROPERTY(QAbstractItemModel* dailyModel READ dailyModel CONSTANT)
  Q_PROPERTY(int airQualityIndex READ airQualityIndex NOTIFY changed)
  Q_PROPERTY(QString airQualityCategory READ airQualityCategory NOTIFY changed)
  Q_PROPERTY(QString localSunset READ localSunset NOTIFY changed)
  Q_PROPERTY(double todayRainProbabilityPercent READ todayRainProbabilityPercent NOTIFY changed)

 public:
  explicit WeatherService(std::optional<WeatherConfig> config, QByteArray openWeatherKey = {},
                          QByteArray ipGeolocationKey = {}, QObject* parent = nullptr);
  WeatherService(WeatherConfig config, std::unique_ptr<WeatherProvider> weather,
                 std::unique_ptr<GeocodingProvider> geocoding, std::unique_ptr<AutomaticLocationProvider> automatic,
                 QObject* parent = nullptr);

  [[nodiscard]] QString state() const { return state_; }
  [[nodiscard]] bool stale() const { return stale_; }
  [[nodiscard]] QDateTime lastSuccessUtc() const { return snapshot_.fetchedUtc; }
  [[nodiscard]] QString diagnostics() const { return diagnostics_; }
  [[nodiscard]] QString city() const { return snapshot_.location.city; }
  [[nodiscard]] QString country() const { return snapshot_.location.country; }
  [[nodiscard]] QString condition() const { return snapshot_.current.condition; }
  [[nodiscard]] QString iconCode() const { return snapshot_.current.iconCode; }
  [[nodiscard]] double temperatureCelsius() const { return snapshot_.current.temperatureCelsius; }
  [[nodiscard]] double feelsLikeCelsius() const { return snapshot_.current.feelsLikeCelsius; }
  [[nodiscard]] double highCelsius() const { return snapshot_.current.highCelsius; }
  [[nodiscard]] double lowCelsius() const { return snapshot_.current.lowCelsius; }
  [[nodiscard]] double humidityPercent() const { return snapshot_.current.humidityPercent; }
  [[nodiscard]] double windSpeedKmh() const { return snapshot_.current.windSpeedKmh; }
  [[nodiscard]] QString windDirection() const;
  [[nodiscard]] QAbstractItemModel* hourlyModel() { return &hourlyModel_; }
  [[nodiscard]] QAbstractItemModel* dailyModel() { return &dailyModel_; }
  [[nodiscard]] int airQualityIndex() const { return snapshot_.airQuality ? snapshot_.airQuality->index : 0; }
  [[nodiscard]] QString airQualityCategory() const {
    return snapshot_.airQuality ? snapshot_.airQuality->category : QString{};
  }
  [[nodiscard]] QString localSunset() const;
  [[nodiscard]] double todayRainProbabilityPercent() const { return snapshot_.todayRainProbabilityPercent; }

  Q_INVOKABLE void refresh();

 signals:
  void changed();

 private:
  enum class Operation : std::uint8_t { None, ResolveLocation, RequestForecast };

  void initialize();
  void startOperation(bool manual);
  void setLocation(const Location& location);
  void publish(Snapshot snapshot);
  void fail(const QString& diagnostic, bool authenticationFailure, int retryAfterSeconds);
  void connectProviders();
  void loadCache();
  void saveCache() const;

  std::optional<WeatherConfig> config_;
  std::unique_ptr<WeatherProvider> weather_;
  std::unique_ptr<GeocodingProvider> geocoding_;
  std::unique_ptr<AutomaticLocationProvider> automatic_;
  Location location_;
  Snapshot snapshot_;
  HourlyForecastModel hourlyModel_;
  DailyForecastModel dailyModel_;
  QTimer* refreshTimer_{};
  QString state_{QStringLiteral("unconfigured")};
  QString diagnostics_;
  bool stale_{};
  bool inFlight_{};
  bool locationResolved_{};
  bool authenticationStopped_{};
  Operation operation_{Operation::None};
  int failureCount_{};
};

}  // namespace dashboard::weather
