#pragma once

#include "weather/weather_types.h"

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QObject>

namespace dashboard::weather {

class GeocodingProvider : public QObject {
  Q_OBJECT
 public:
  using QObject::QObject;
  ~GeocodingProvider() override = default;
  Q_DISABLE_COPY_MOVE(GeocodingProvider)
  virtual void resolve(const QString& city) = 0;

 signals:
  void resolved(const dashboard::weather::Location& location);
  void failed(const QString& diagnostic, bool authenticationFailure, int retryAfterSeconds);
};

class AutomaticLocationProvider : public QObject {
  Q_OBJECT
 public:
  using QObject::QObject;
  ~AutomaticLocationProvider() override = default;
  Q_DISABLE_COPY_MOVE(AutomaticLocationProvider)
  virtual void resolve() = 0;

 signals:
  void resolved(const dashboard::weather::Location& location);
  void failed(const QString& diagnostic, bool authenticationFailure, int retryAfterSeconds);
};

class WeatherProvider : public QObject {
  Q_OBJECT
 public:
  using QObject::QObject;
  ~WeatherProvider() override = default;
  Q_DISABLE_COPY_MOVE(WeatherProvider)
  virtual void request(const Location& location) = 0;

 signals:
  void forecastReady(const dashboard::weather::Snapshot& snapshot);
  void airQualityReady(const dashboard::weather::AirQuality& airQuality);
  void airQualityFailed(const QString& diagnostic);
  void failed(const QString& diagnostic, bool authenticationFailure, int retryAfterSeconds);
};

class OpenWeatherProvider final : public WeatherProvider {
  Q_OBJECT
 public:
  explicit OpenWeatherProvider(QByteArray apiKey, QObject* parent = nullptr);
  void request(const Location& location) override;

  [[nodiscard]] static Snapshot parseOneCall(const QJsonDocument& document, const Location& location);
  [[nodiscard]] static AirQuality parseAirQuality(const QJsonDocument& document);

 private:
  void handleNetworkFailure(QNetworkReply* reply, const QString& operation, bool airQuality = false);
  [[nodiscard]] static QNetworkRequest requestFor(const QUrl& url);

  QByteArray apiKey_;
  QNetworkAccessManager network_;
};

class OpenWeatherGeocodingProvider final : public GeocodingProvider {
  Q_OBJECT
 public:
  explicit OpenWeatherGeocodingProvider(QByteArray apiKey, QObject* parent = nullptr);
  void resolve(const QString& city) override;
  [[nodiscard]] static Location parseGeocoding(const QJsonDocument& document);

 private:
  QByteArray apiKey_;
  QNetworkAccessManager network_;
};

class IpGeolocationProvider final : public AutomaticLocationProvider {
  Q_OBJECT
 public:
  explicit IpGeolocationProvider(QByteArray apiKey, QObject* parent = nullptr);
  void resolve() override;
  [[nodiscard]] static Location parseLocation(const QJsonDocument& document);

 private:
  QByteArray apiKey_;
  QNetworkAccessManager network_;
};

}  // namespace dashboard::weather

Q_DECLARE_METATYPE(dashboard::weather::Location)
Q_DECLARE_METATYPE(dashboard::weather::AirQuality)
Q_DECLARE_METATYPE(dashboard::weather::Snapshot)
