#include "weather/weather_provider.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QTimeZone>
#include <QUrlQuery>

#include <algorithm>
#include <stdexcept>

namespace dashboard::weather {
namespace {

constexpr int kTransferTimeoutMilliseconds = 15000;

double requiredNumber(const QJsonObject& object, const char* name) {
  const auto value = object.value(QLatin1String(name));
  if (!value.isDouble()) {
    throw std::runtime_error("weather response is missing a numeric field");
  }
  return value.toDouble();
}

QDateTime timestamp(double seconds) {
  return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(seconds), QTimeZone::UTC);
}

QString aqiCategory(int index) {
  static const QStringList categories{QStringLiteral("Good"), QStringLiteral("Fair"), QStringLiteral("Moderate"),
                                      QStringLiteral("Poor"), QStringLiteral("Very poor")};
  return index >= 1 && index <= categories.size() ? categories.at(index - 1) : QStringLiteral("Unavailable");
}

QString replyDiagnostic(QNetworkReply* reply, const QString& operation) {
  const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if (status == 401 || status == 403) {
    return QStringLiteral("%1 authentication failed").arg(operation);
  }
  if (status == 429) {
    return QStringLiteral("%1 rate limit reached").arg(operation);
  }
  return QStringLiteral("%1 request failed").arg(operation);
}

}  // namespace

QString Location::cacheKey() const {
  return QStringLiteral("%1,%2").arg(latitude, 0, 'f', 4).arg(longitude, 0, 'f', 4);
}

OpenWeatherProvider::OpenWeatherProvider(QByteArray apiKey, QObject* parent)
    : WeatherProvider(parent), apiKey_(std::move(apiKey)) {}

QNetworkRequest OpenWeatherProvider::requestFor(const QUrl& url) {
  QNetworkRequest request(url);
  request.setTransferTimeout(kTransferTimeoutMilliseconds);
  return request;
}

OpenWeatherGeocodingProvider::OpenWeatherGeocodingProvider(QByteArray apiKey, QObject* parent)
    : GeocodingProvider(parent), apiKey_(std::move(apiKey)) {}

void OpenWeatherGeocodingProvider::resolve(const QString& city) {
  QUrl url(QStringLiteral("https://api.openweathermap.org/geo/1.0/direct"));
  QUrlQuery query;
  query.addQueryItem(QStringLiteral("q"), city);
  query.addQueryItem(QStringLiteral("limit"), QStringLiteral("1"));
  query.addQueryItem(QStringLiteral("appid"), QString::fromUtf8(apiKey_));
  url.setQuery(query);
  QNetworkRequest request(url);
  request.setTransferTimeout(kTransferTimeoutMilliseconds);
  auto* reply = network_.get(request);
  connect(reply, &QNetworkReply::finished, this, [this, reply] {
    if (reply->error() != QNetworkReply::NoError) {
      const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
      emit failed(replyDiagnostic(reply, QStringLiteral("Geocoding")), status == 401 || status == 403, 0);
      reply->deleteLater();
      return;
    }
    try {
      emit resolved(parseGeocoding(QJsonDocument::fromJson(reply->readAll())));
    } catch (const std::exception&) {
      emit failed(QStringLiteral("Geocoding response was invalid"), false, 0);
    }
    reply->deleteLater();
  });
}

void OpenWeatherProvider::request(const Location& location) {
  auto makeUrl = [&](const QString& path) {
    QUrl url(path);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("lat"), QString::number(location.latitude, 'f', 6));
    query.addQueryItem(QStringLiteral("lon"), QString::number(location.longitude, 'f', 6));
    query.addQueryItem(QStringLiteral("appid"), QString::fromUtf8(apiKey_));
    if (path.contains(QStringLiteral("onecall"))) {
      query.addQueryItem(QStringLiteral("units"), QStringLiteral("metric"));
      query.addQueryItem(QStringLiteral("exclude"), QStringLiteral("minutely,alerts"));
    }
    url.setQuery(query);
    return url;
  };

  auto* forecast = network_.get(requestFor(makeUrl(QStringLiteral("https://api.openweathermap.org/data/3.0/onecall"))));
  connect(forecast, &QNetworkReply::finished, this, [this, forecast, location] {
    if (forecast->error() != QNetworkReply::NoError) {
      handleNetworkFailure(forecast, QStringLiteral("Weather"));
      return;
    }
    try {
      emit forecastReady(parseOneCall(QJsonDocument::fromJson(forecast->readAll()), location));
    } catch (const std::exception&) {
      emit failed(QStringLiteral("Weather response was invalid"), false, 0);
    }
    forecast->deleteLater();
  });

  auto* air =
      network_.get(requestFor(makeUrl(QStringLiteral("https://api.openweathermap.org/data/2.5/air_pollution"))));
  connect(air, &QNetworkReply::finished, this, [this, air] {
    if (air->error() != QNetworkReply::NoError) {
      handleNetworkFailure(air, QStringLiteral("Air quality"), true);
      return;
    }
    try {
      emit airQualityReady(parseAirQuality(QJsonDocument::fromJson(air->readAll())));
    } catch (const std::exception&) {
      emit airQualityFailed(QStringLiteral("Air quality response was invalid"));
    }
    air->deleteLater();
  });
}

void OpenWeatherProvider::handleNetworkFailure(QNetworkReply* reply, const QString& operation, bool airQuality) {
  const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  const bool authentication = status == 401 || status == 403;
  bool retryValid = false;
  const int retryAfter = QString::fromLatin1(reply->rawHeader("Retry-After")).toInt(&retryValid);
  const QString diagnostic = replyDiagnostic(reply, operation);
  if (airQuality) {
    emit airQualityFailed(diagnostic);
  } else {
    emit failed(diagnostic, authentication, retryValid ? retryAfter : 0);
  }
  reply->deleteLater();
}

Location OpenWeatherGeocodingProvider::parseGeocoding(const QJsonDocument& document) {
  const auto array = document.array();
  if (array.isEmpty()) {
    throw std::runtime_error("location not found");
  }
  const auto object = array.first().toObject();
  return {.city = object.value(QStringLiteral("name")).toString(),
          .country = object.value(QStringLiteral("country")).toString(),
          .latitude = requiredNumber(object, "lat"),
          .longitude = requiredNumber(object, "lon")};
}

Snapshot OpenWeatherProvider::parseOneCall(const QJsonDocument& document, const Location& location) {
  const auto root = document.object();
  const auto current = root.value(QStringLiteral("current")).toObject();
  const auto today = root.value(QStringLiteral("daily")).toArray().at(0).toObject();
  const auto currentWeather = current.value(QStringLiteral("weather")).toArray().at(0).toObject();
  Snapshot snapshot{.provider = QStringLiteral("openweather"),
                    .location = location,
                    .timezone = root.value(QStringLiteral("timezone")).toString(),
                    .timezoneOffsetSeconds = root.value(QStringLiteral("timezone_offset")).toInt(),
                    .fetchedUtc = QDateTime::currentDateTimeUtc()};
  snapshot.current = {
      .observedUtc = timestamp(requiredNumber(current, "dt")),
      .condition = currentWeather.value(QStringLiteral("description")).toString(),
      .iconCode = currentWeather.value(QStringLiteral("icon")).toString(),
      .temperatureCelsius = requiredNumber(current, "temp"),
      .feelsLikeCelsius = requiredNumber(current, "feels_like"),
      .highCelsius = today.value(QStringLiteral("temp")).toObject().value(QStringLiteral("max")).toDouble(),
      .lowCelsius = today.value(QStringLiteral("temp")).toObject().value(QStringLiteral("min")).toDouble(),
      .humidityPercent = requiredNumber(current, "humidity"),
      .pressureHpa = requiredNumber(current, "pressure"),
      .windSpeedKmh = requiredNumber(current, "wind_speed") * 3.6,
      .windDegrees = requiredNumber(current, "wind_deg")};
  snapshot.sunsetUtc = timestamp(today.value(QStringLiteral("sunset")).toDouble());
  snapshot.todayRainProbabilityPercent = today.value(QStringLiteral("pop")).toDouble() * 100.0;
  const auto hourly = root.value(QStringLiteral("hourly")).toArray();
  for (const auto& value : hourly) {
    const auto object = value.toObject();
    const auto weather = object.value(QStringLiteral("weather")).toArray().at(0).toObject();
    snapshot.hourly.push_back(
        {.timestampUtc = timestamp(requiredNumber(object, "dt")),
         .iconCode = weather.value(QStringLiteral("icon")).toString(),
         .temperatureCelsius = requiredNumber(object, "temp"),
         .precipitationProbabilityPercent = object.value(QStringLiteral("pop")).toDouble() * 100.0});
  }
  const auto daily = root.value(QStringLiteral("daily")).toArray();
  for (const auto& value : daily) {
    const auto object = value.toObject();
    const auto temperatures = object.value(QStringLiteral("temp")).toObject();
    const auto weather = object.value(QStringLiteral("weather")).toArray().at(0).toObject();
    snapshot.daily.push_back(
        {.timestampUtc = timestamp(requiredNumber(object, "dt")),
         .iconCode = weather.value(QStringLiteral("icon")).toString(),
         .minimumCelsius = requiredNumber(temperatures, "min"),
         .maximumCelsius = requiredNumber(temperatures, "max"),
         .precipitationProbabilityPercent = object.value(QStringLiteral("pop")).toDouble() * 100.0});
  }
  return snapshot;
}

AirQuality OpenWeatherProvider::parseAirQuality(const QJsonDocument& document) {
  const auto entry = document.object().value(QStringLiteral("list")).toArray().at(0).toObject();
  const int index = entry.value(QStringLiteral("main")).toObject().value(QStringLiteral("aqi")).toInt();
  if (index < 1 || index > 5) {
    throw std::runtime_error("invalid aqi");
  }
  return {.index = index,
          .category = aqiCategory(index),
          .pm25MicrogramsPerCubicMetre =
              entry.value(QStringLiteral("components")).toObject().value(QStringLiteral("pm2_5")).toDouble()};
}

IpGeolocationProvider::IpGeolocationProvider(QByteArray apiKey, QObject* parent)
    : AutomaticLocationProvider(parent), apiKey_(std::move(apiKey)) {}

void IpGeolocationProvider::resolve() {
  QUrl url(QStringLiteral("https://api.ipgeolocation.io/ipgeo"));
  QUrlQuery query;
  query.addQueryItem(QStringLiteral("apiKey"), QString::fromUtf8(apiKey_));
  query.addQueryItem(QStringLiteral("fields"), QStringLiteral("city,country_code2,latitude,longitude"));
  url.setQuery(query);
  QNetworkRequest request(url);
  request.setTransferTimeout(kTransferTimeoutMilliseconds);
  auto* reply = network_.get(request);
  connect(reply, &QNetworkReply::finished, this, [this, reply] {
    if (reply->error() != QNetworkReply::NoError) {
      const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
      emit failed(QStringLiteral("Automatic location request failed"), status == 401 || status == 403, 0);
    } else {
      try {
        emit resolved(parseLocation(QJsonDocument::fromJson(reply->readAll())));
      } catch (const std::exception&) {
        emit failed(QStringLiteral("Automatic location response was invalid"), false, 0);
      }
    }
    reply->deleteLater();
  });
}

Location IpGeolocationProvider::parseLocation(const QJsonDocument& document) {
  const auto object = document.object();
  bool latitudeValid = false;
  bool longitudeValid = false;
  const double latitude = object.value(QStringLiteral("latitude")).toVariant().toDouble(&latitudeValid);
  const double longitude = object.value(QStringLiteral("longitude")).toVariant().toDouble(&longitudeValid);
  if (!latitudeValid || !longitudeValid || latitude < -90.0 || latitude > 90.0 || longitude < -180.0 ||
      longitude > 180.0) {
    throw std::runtime_error("invalid automatic location");
  }
  return {.city = object.value(QStringLiteral("city")).toString(),
          .country = object.value(QStringLiteral("country_code2")).toString(),
          .latitude = latitude,
          .longitude = longitude};
}

}  // namespace dashboard::weather
