#include "weather/weather_service.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>
#include <cmath>

namespace dashboard::weather {
// NOLINTBEGIN(readability-braces-around-statements,cppcoreguidelines-narrowing-conversions,performance-unnecessary-value-param)
namespace {
constexpr int kCacheStaleSeconds = 1200;

QString cachePath() {
  return QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath(QStringLiteral("weather.json"));
}

QJsonObject locationJson(const Location& location) {
  return {{QStringLiteral("city"), location.city},
          {QStringLiteral("country"), location.country},
          {QStringLiteral("latitude"), location.latitude},
          {QStringLiteral("longitude"), location.longitude}};
}
Location jsonLocation(const QJsonObject& object) {
  return {.city = object.value(QStringLiteral("city")).toString(),
          .country = object.value(QStringLiteral("country")).toString(),
          .latitude = object.value(QStringLiteral("latitude")).toDouble(),
          .longitude = object.value(QStringLiteral("longitude")).toDouble()};
}

QString sanitizedDiagnostic(QString diagnostic) {
  static const QRegularExpression urlExpression(QStringLiteral(R"(https?://[^\s]+)"));
  qsizetype offset = 0;
  while (true) {
    const auto match = urlExpression.match(diagnostic, offset);
    if (!match.hasMatch()) break;
    QUrl url(match.captured());
    url.setQuery(QString{});
    url.setFragment(QString{});
    const QString sanitized = url.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);
    diagnostic.replace(match.capturedStart(), match.capturedLength(), sanitized);
    offset = match.capturedStart() + sanitized.size();
  }
  return diagnostic;
}
}  // namespace

WeatherService::WeatherService(std::optional<WeatherConfig> config, QByteArray openWeatherKey,
                               QByteArray ipGeolocationKey, QObject* parent)
    : QObject(parent), config_(std::move(config)), hourlyModel_(this), dailyModel_(this) {
  if (config_) {
    weather_ = std::make_unique<OpenWeatherProvider>(openWeatherKey);
    geocoding_ = std::make_unique<OpenWeatherGeocodingProvider>(openWeatherKey);
    automatic_ = std::make_unique<IpGeolocationProvider>(ipGeolocationKey);
    if (openWeatherKey.isEmpty()) diagnostics_ = QStringLiteral("OpenWeather credential is unavailable");
    if (config_->locationMode == LocationMode::Automatic && ipGeolocationKey.isEmpty())
      diagnostics_ = QStringLiteral("Automatic location credential is unavailable");
  }
  initialize();
}

WeatherService::WeatherService(WeatherConfig config, std::unique_ptr<WeatherProvider> weather,
                               std::unique_ptr<GeocodingProvider> geocoding,
                               std::unique_ptr<AutomaticLocationProvider> automatic, QObject* parent)
    : QObject(parent),
      config_(std::move(config)),
      weather_(std::move(weather)),
      geocoding_(std::move(geocoding)),
      automatic_(std::move(automatic)),
      hourlyModel_(this),
      dailyModel_(this) {
  initialize();
}

void WeatherService::initialize() {
  refreshTimer_ = new QTimer(this);
  refreshTimer_->setSingleShot(true);
  refreshTimer_->setObjectName(QStringLiteral("weatherRefreshTimer"));
  connect(refreshTimer_, &QTimer::timeout, this, [this] { startOperation(false); });
  if (!config_) return;
  connectProviders();
  loadCache();
  if (!diagnostics_.isEmpty()) {
    state_ = QStringLiteral("error");
    emit changed();
    return;
  }
  if (config_->locationMode == LocationMode::Coordinates) {
    setLocation({.latitude = config_->latitude, .longitude = config_->longitude});
  } else {
    startOperation(false);
  }
  emit changed();
}

void WeatherService::connectProviders() {
  connect(weather_.get(), &WeatherProvider::forecastReady, this,
          [this](const Snapshot& snapshot) { publish(snapshot); });
  connect(weather_.get(), &WeatherProvider::airQualityReady, this, [this](const AirQuality& airQuality) {
    snapshot_.airQuality = airQuality;
    emit changed();
    saveCache();
  });
  connect(weather_.get(), &WeatherProvider::airQualityFailed, this, [this](const QString& diagnostic) {
    diagnostics_ = sanitizedDiagnostic(diagnostic);
    qWarning().noquote() << diagnostics_;
    emit changed();
  });
  connect(weather_.get(), &WeatherProvider::failed, this, &WeatherService::fail);
  connect(geocoding_.get(), &GeocodingProvider::resolved, this, &WeatherService::setLocation);
  connect(geocoding_.get(), &GeocodingProvider::failed, this, &WeatherService::fail);
  connect(automatic_.get(), &AutomaticLocationProvider::resolved, this, &WeatherService::setLocation);
  connect(automatic_.get(), &AutomaticLocationProvider::failed, this, &WeatherService::fail);
}

void WeatherService::setLocation(const Location& location) {
  inFlight_ = false;
  operation_ = Operation::None;
  location_ = location;
  locationResolved_ = true;
  if (snapshot_.fetchedUtc.isValid() && snapshot_.location.cacheKey() != location.cacheKey()) {
    snapshot_ = {};
    hourlyModel_.replace({}, 0);
    dailyModel_.replace({}, 0);
    stale_ = false;
  }
  startOperation(false);
}

void WeatherService::refresh() { startOperation(true); }

void WeatherService::startOperation(bool manual) {
  if (!config_ || inFlight_ || !weather_) return;
  if (!manual && authenticationStopped_) return;
  if (manual) {
    authenticationStopped_ = false;
    refreshTimer_->stop();
  }
  inFlight_ = true;
  operation_ = locationResolved_ ? Operation::RequestForecast : Operation::ResolveLocation;
  state_ = locationResolved_ ? QStringLiteral("loading") : QStringLiteral("locating");
  if (snapshot_.fetchedUtc.isValid()) stale_ = true;
  diagnostics_.clear();
  emit changed();
  if (operation_ == Operation::RequestForecast) {
    weather_->request(location_);
  } else if (config_->locationMode == LocationMode::City) {
    geocoding_->resolve(config_->city);
  } else {
    automatic_->resolve();
  }
}

void WeatherService::publish(Snapshot snapshot) {
  if (snapshot.airQuality == std::nullopt && snapshot_.location.cacheKey() == snapshot.location.cacheKey())
    snapshot.airQuality = snapshot_.airQuality;
  snapshot_ = std::move(snapshot);
  hourlyModel_.replace(snapshot_.hourly, snapshot_.timezoneOffsetSeconds);
  dailyModel_.replace(snapshot_.daily, snapshot_.timezoneOffsetSeconds);
  inFlight_ = false;
  operation_ = Operation::None;
  stale_ = false;
  failureCount_ = 0;
  state_ = QStringLiteral("ready");
  diagnostics_.clear();
  refreshTimer_->start(config_->refreshIntervalSeconds * 1000);
  saveCache();
  emit changed();
}

void WeatherService::fail(const QString& diagnostic, bool authenticationFailure, int retryAfterSeconds) {
  inFlight_ = false;
  operation_ = Operation::None;
  diagnostics_ = sanitizedDiagnostic(diagnostic);
  qWarning().noquote() << diagnostics_;
  stale_ = snapshot_.fetchedUtc.isValid();
  state_ = QStringLiteral("error");
  authenticationStopped_ = authenticationFailure;
  if (!authenticationFailure) {
    ++failureCount_;
    const int backoff = std::min(config_->refreshIntervalSeconds, 30 * (1 << std::min(failureCount_ - 1, 5)));
    refreshTimer_->start(std::max(backoff, retryAfterSeconds) * 1000);
  }
  emit changed();
}

QString WeatherService::windDirection() const {
  static const QStringList points{QStringLiteral("N"), QStringLiteral("NE"), QStringLiteral("E"), QStringLiteral("SE"),
                                  QStringLiteral("S"), QStringLiteral("SW"), QStringLiteral("W"), QStringLiteral("NW")};
  const int index = static_cast<int>(std::lround(snapshot_.current.windDegrees / 45.0)) % points.size();
  return points.at(index);
}

QString WeatherService::localSunset() const {
  if (!snapshot_.sunsetUtc.isValid()) return {};
  return QLocale().toString(snapshot_.sunsetUtc.addSecs(snapshot_.timezoneOffsetSeconds).time(), QLocale::ShortFormat);
}

void WeatherService::saveCache() const {
  if (!snapshot_.fetchedUtc.isValid()) return;
  QJsonObject current{{QStringLiteral("condition"), snapshot_.current.condition},
                      {QStringLiteral("icon"), snapshot_.current.iconCode},
                      {QStringLiteral("temperature"), snapshot_.current.temperatureCelsius},
                      {QStringLiteral("feelsLike"), snapshot_.current.feelsLikeCelsius},
                      {QStringLiteral("high"), snapshot_.current.highCelsius},
                      {QStringLiteral("low"), snapshot_.current.lowCelsius},
                      {QStringLiteral("humidity"), snapshot_.current.humidityPercent},
                      {QStringLiteral("windSpeed"), snapshot_.current.windSpeedKmh},
                      {QStringLiteral("windDegrees"), snapshot_.current.windDegrees}};
  QJsonArray hourly;
  for (const auto& row : snapshot_.hourly.mid(0, 8))
    hourly.append(QJsonObject{{QStringLiteral("timestamp"), row.timestampUtc.toString(Qt::ISODate)},
                              {QStringLiteral("icon"), row.iconCode},
                              {QStringLiteral("temperature"), row.temperatureCelsius},
                              {QStringLiteral("precipitation"), row.precipitationProbabilityPercent}});
  QJsonArray daily;
  for (const auto& row : snapshot_.daily.mid(0, 5))
    daily.append(QJsonObject{{QStringLiteral("timestamp"), row.timestampUtc.toString(Qt::ISODate)},
                             {QStringLiteral("icon"), row.iconCode},
                             {QStringLiteral("minimum"), row.minimumCelsius},
                             {QStringLiteral("maximum"), row.maximumCelsius},
                             {QStringLiteral("precipitation"), row.precipitationProbabilityPercent}});
  QJsonObject root{{QStringLiteral("provider"), snapshot_.provider},
                   {QStringLiteral("location"), locationJson(snapshot_.location)},
                   {QStringLiteral("timezoneOffset"), snapshot_.timezoneOffsetSeconds},
                   {QStringLiteral("fetched"), snapshot_.fetchedUtc.toString(Qt::ISODate)},
                   {QStringLiteral("sunset"), snapshot_.sunsetUtc.toString(Qt::ISODate)},
                   {QStringLiteral("rainProbability"), snapshot_.todayRainProbabilityPercent},
                   {QStringLiteral("current"), current},
                   {QStringLiteral("hourly"), hourly},
                   {QStringLiteral("daily"), daily}};
  if (snapshot_.airQuality)
    root.insert(QStringLiteral("airQuality"),
                QJsonObject{{QStringLiteral("index"), snapshot_.airQuality->index},
                            {QStringLiteral("category"), snapshot_.airQuality->category}});
  QDir().mkpath(QFileInfo(cachePath()).absolutePath());
  QSaveFile file(cachePath());
  if (file.open(QIODevice::WriteOnly)) {
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    file.commit();
  }
}

void WeatherService::loadCache() {
  QFile file(cachePath());
  if (!file.open(QIODevice::ReadOnly)) return;
  const auto root = QJsonDocument::fromJson(file.readAll()).object();
  if (root.value(QStringLiteral("provider")).toString() != config_->provider) return;
  Snapshot cached{.provider = config_->provider,
                  .location = jsonLocation(root.value(QStringLiteral("location")).toObject()),
                  .timezoneOffsetSeconds = root.value(QStringLiteral("timezoneOffset")).toInt(),
                  .fetchedUtc = QDateTime::fromString(root.value(QStringLiteral("fetched")).toString(), Qt::ISODate)};
  if (config_->locationMode == LocationMode::Coordinates &&
      cached.location.cacheKey() != Location{.latitude = config_->latitude, .longitude = config_->longitude}.cacheKey())
    return;
  if (config_->locationMode == LocationMode::City &&
      cached.location.city.compare(config_->city.section(',', 0, 0), Qt::CaseInsensitive) != 0)
    return;
  const auto current = root.value(QStringLiteral("current")).toObject();
  cached.current = {.condition = current.value(QStringLiteral("condition")).toString(),
                    .iconCode = current.value(QStringLiteral("icon")).toString(),
                    .temperatureCelsius = current.value(QStringLiteral("temperature")).toDouble(),
                    .feelsLikeCelsius = current.value(QStringLiteral("feelsLike")).toDouble(),
                    .highCelsius = current.value(QStringLiteral("high")).toDouble(),
                    .lowCelsius = current.value(QStringLiteral("low")).toDouble(),
                    .humidityPercent = current.value(QStringLiteral("humidity")).toDouble(),
                    .windSpeedKmh = current.value(QStringLiteral("windSpeed")).toDouble(),
                    .windDegrees = current.value(QStringLiteral("windDegrees")).toDouble()};
  for (const auto& value : root.value(QStringLiteral("hourly")).toArray()) {
    const auto row = value.toObject();
    cached.hourly.push_back(
        {.timestampUtc = QDateTime::fromString(row.value(QStringLiteral("timestamp")).toString(), Qt::ISODate),
         .iconCode = row.value(QStringLiteral("icon")).toString(),
         .temperatureCelsius = row.value(QStringLiteral("temperature")).toDouble(),
         .precipitationProbabilityPercent = row.value(QStringLiteral("precipitation")).toDouble()});
  }
  for (const auto& value : root.value(QStringLiteral("daily")).toArray()) {
    const auto row = value.toObject();
    cached.daily.push_back(
        {.timestampUtc = QDateTime::fromString(row.value(QStringLiteral("timestamp")).toString(), Qt::ISODate),
         .iconCode = row.value(QStringLiteral("icon")).toString(),
         .minimumCelsius = row.value(QStringLiteral("minimum")).toDouble(),
         .maximumCelsius = row.value(QStringLiteral("maximum")).toDouble(),
         .precipitationProbabilityPercent = row.value(QStringLiteral("precipitation")).toDouble()});
  }
  cached.sunsetUtc = QDateTime::fromString(root.value(QStringLiteral("sunset")).toString(), Qt::ISODate);
  cached.todayRainProbabilityPercent = root.value(QStringLiteral("rainProbability")).toDouble();
  const auto air = root.value(QStringLiteral("airQuality")).toObject();
  if (!air.isEmpty())
    cached.airQuality = AirQuality{.index = air.value(QStringLiteral("index")).toInt(),
                                   .category = air.value(QStringLiteral("category")).toString()};
  snapshot_ = cached;
  hourlyModel_.replace(cached.hourly, cached.timezoneOffsetSeconds);
  dailyModel_.replace(cached.daily, cached.timezoneOffsetSeconds);
  stale_ = cached.fetchedUtc.secsTo(QDateTime::currentDateTimeUtc()) > kCacheStaleSeconds;
  state_ = QStringLiteral("ready");
}

// NOLINTEND(readability-braces-around-statements,cppcoreguidelines-narrowing-conversions,performance-unnecessary-value-param)
}  // namespace dashboard::weather
