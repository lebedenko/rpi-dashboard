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
#include <array>
#include <cmath>
#include <limits>

namespace dashboard::weather {
// NOLINTBEGIN(readability-braces-around-statements,cppcoreguidelines-narrowing-conversions,performance-unnecessary-value-param)
namespace {
constexpr int kCacheStaleSeconds = 1200;

bool isSupportedIconCode(const QString& code) {
  static const QRegularExpression expression(QStringLiteral(R"(^(01|02|03|04|09|10|11|13|50)[dn]$)"));
  return expression.match(code).hasMatch();
}

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
    : QObject(parent),
      config_(std::move(config)),
      hourlyModel_(this),
      dailyModel_(this),
      currentUtc_(QDateTime::currentDateTimeUtc) {
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
                               std::unique_ptr<AutomaticLocationProvider> automatic, QObject* parent,
                               std::function<QDateTime()> currentUtc)
    : QObject(parent),
      config_(std::move(config)),
      weather_(std::move(weather)),
      geocoding_(std::move(geocoding)),
      automatic_(std::move(automatic)),
      hourlyModel_(this),
      dailyModel_(this),
      currentUtc_(std::move(currentUtc)) {
  initialize();
}

void WeatherService::initialize() {
  refreshTimer_ = new QTimer(this);
  refreshTimer_->setSingleShot(true);
  refreshTimer_->setObjectName(QStringLiteral("weatherRefreshTimer"));
  connect(refreshTimer_, &QTimer::timeout, this, [this] { startOperation(false); });
  solarEventTimer_ = new QTimer(this);
  solarEventTimer_->setSingleShot(true);
  solarEventTimer_->setObjectName(QStringLiteral("weatherSolarEventTimer"));
  connect(solarEventTimer_, &QTimer::timeout, this, &WeatherService::updateNextSolarEvent);
  localHourTimer_ = new QTimer(this);
  localHourTimer_->setSingleShot(true);
  localHourTimer_->setObjectName(QStringLiteral("weatherLocalHourTimer"));
  connect(localHourTimer_, &QTimer::timeout, this, [this] {
    const double previous = snapshot_.todayPrecipitationProbabilityPercent;
    updateTodayPrecipitationProbability();
    scheduleLocalHourBoundary();
    if (snapshot_.todayPrecipitationProbabilityPercent != previous) emit changed();
  });
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
    updateNextSolarEvent();
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
  if (snapshot.todayPrecipitationKind.isEmpty())
    snapshot.todayPrecipitationKind = QStringLiteral("none");
  if (snapshot.airQuality == std::nullopt && snapshot_.location.cacheKey() == snapshot.location.cacheKey())
    snapshot.airQuality = snapshot_.airQuality;
  snapshot_ = std::move(snapshot);
  updateTodayPrecipitationProbability();
  scheduleLocalHourBoundary();
  hourlyModel_.replace(snapshot_.hourly, snapshot_.timezoneOffsetSeconds);
  dailyModel_.replace(snapshot_.daily, snapshot_.timezoneOffsetSeconds);
  updateNextSolarEvent();
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

QString WeatherService::localNextSolarEventTime() const {
  if (!nextSolarEventUtc_.isValid()) return {};
  return QLocale().toString(nextSolarEventUtc_.addSecs(snapshot_.timezoneOffsetSeconds).time(), QLocale::ShortFormat);
}

void WeatherService::updateNextSolarEvent() {
  const auto now = currentUtc_();
  QDateTime selectedTimestamp;
  QString selectedKind;
  for (const auto& day : snapshot_.daily) {
    const std::array events{std::pair{day.sunriseUtc, QStringLiteral("sunrise")},
                            std::pair{day.sunsetUtc, QStringLiteral("sunset")}};
    for (const auto& [timestampUtc, kind] : events) {
      if (timestampUtc.isValid() && timestampUtc > now &&
          (!selectedTimestamp.isValid() || timestampUtc < selectedTimestamp)) {
        selectedTimestamp = timestampUtc;
        selectedKind = kind;
      }
    }
  }
  if (!selectedTimestamp.isValid()) {
    const QString iconCode =
        isSupportedIconCode(snapshot_.current.iconCode) ? snapshot_.current.iconCode : QStringLiteral("03d");
    selectedKind = iconCode.endsWith(QLatin1Char('n')) ? QStringLiteral("sunrise") : QStringLiteral("sunset");
  }

  const bool changed = selectedTimestamp != nextSolarEventUtc_ || selectedKind != nextSolarEventKind_;
  nextSolarEventUtc_ = selectedTimestamp;
  nextSolarEventKind_ = selectedKind;
  solarEventTimer_->stop();
  if (selectedTimestamp.isValid()) {
    const qint64 delay = std::max<qint64>(1, now.msecsTo(selectedTimestamp) + 1);
    solarEventTimer_->start(static_cast<int>(std::min<qint64>(delay, std::numeric_limits<int>::max())));
  }
  if (changed) emit solarEventChanged();
}

void WeatherService::updateTodayPrecipitationProbability() {
  const QDateTime now = currentUtc_();
  const QDateTime localNow = now.addSecs(snapshot_.timezoneOffsetSeconds);
  const QDate localDate = localNow.date();
  const int currentHour = localNow.time().hour();
  double maximum = 0.0;
  for (const auto& row : snapshot_.hourly) {
    if (!row.timestampUtc.isValid()) continue;
    const QDateTime localTimestamp = row.timestampUtc.addSecs(snapshot_.timezoneOffsetSeconds);
    if (localTimestamp.date() == localDate && localTimestamp.time().hour() >= currentHour)
      maximum = std::max(maximum, row.precipitationProbabilityPercent);
  }
  snapshot_.todayPrecipitationProbabilityPercent = maximum;
  snapshot_.todayRainProbabilityPercent = maximum;
}

void WeatherService::scheduleLocalHourBoundary() {
  localHourTimer_->stop();
  if (!snapshot_.fetchedUtc.isValid()) return;
  const QDateTime now = currentUtc_();
  const QDateTime localNow = now.addSecs(snapshot_.timezoneOffsetSeconds);
  const QDateTime nextLocalHour(localNow.date(), QTime(localNow.time().hour(), 0), QTimeZone::UTC);
  const qint64 delay = localNow.msecsTo(nextLocalHour.addSecs(3600)) + 1;
  localHourTimer_->start(static_cast<int>(std::clamp<qint64>(delay, 1, std::numeric_limits<int>::max())));
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
  for (const auto& row : snapshot_.hourly)
    hourly.append(QJsonObject{{QStringLiteral("timestamp"), row.timestampUtc.toString(Qt::ISODate)},
                              {QStringLiteral("icon"), row.iconCode},
                              {QStringLiteral("temperature"), row.temperatureCelsius},
                              {QStringLiteral("precipitation"), row.precipitationProbabilityPercent}});
  QJsonArray daily;
  for (const auto& row : snapshot_.daily.mid(0, 5))
    daily.append(
        QJsonObject{{QStringLiteral("timestamp"), row.timestampUtc.toString(Qt::ISODate)},
                    {QStringLiteral("sunrise"), row.sunriseUtc.toString(Qt::ISODate)},
                    {QStringLiteral("sunset"), row.sunsetUtc.toString(Qt::ISODate)},
                    {QStringLiteral("icon"), row.iconCode},
                    {QStringLiteral("minimum"), row.minimumCelsius},
                    {QStringLiteral("maximum"), row.maximumCelsius},
                    {QStringLiteral("precipitation"), row.precipitationProbabilityPercent},
                    {QStringLiteral("average"), row.averageCelsius ? QJsonValue(*row.averageCelsius) : QJsonValue()},
                    {QStringLiteral("rain"), row.rainMillimetres ? QJsonValue(*row.rainMillimetres) : QJsonValue()},
                    {QStringLiteral("snow"), row.snowMillimetres ? QJsonValue(*row.snowMillimetres) : QJsonValue()}});
  QJsonObject root{{QStringLiteral("provider"), snapshot_.provider},
                   {QStringLiteral("location"), locationJson(snapshot_.location)},
                   {QStringLiteral("timezoneOffset"), snapshot_.timezoneOffsetSeconds},
                   {QStringLiteral("fetched"), snapshot_.fetchedUtc.toString(Qt::ISODate)},
                   {QStringLiteral("sunset"), snapshot_.sunsetUtc.toString(Qt::ISODate)},
                   {QStringLiteral("precipitationKind"), snapshot_.todayPrecipitationKind},
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
         .sunriseUtc = QDateTime::fromString(row.value(QStringLiteral("sunrise")).toString(), Qt::ISODate),
         .sunsetUtc = QDateTime::fromString(row.value(QStringLiteral("sunset")).toString(), Qt::ISODate),
         .iconCode = row.value(QStringLiteral("icon")).toString(),
         .minimumCelsius = row.value(QStringLiteral("minimum")).toDouble(),
         .maximumCelsius = row.value(QStringLiteral("maximum")).toDouble(),
         .averageCelsius = row.value(QStringLiteral("average")).isDouble()
                               ? std::optional<double>{row.value(QStringLiteral("average")).toDouble()}
                               : std::nullopt,
         .precipitationProbabilityPercent = row.value(QStringLiteral("precipitation")).toDouble(),
         .rainMillimetres = row.value(QStringLiteral("rain")).isDouble()
                                ? std::optional<double>{row.value(QStringLiteral("rain")).toDouble()}
                                : std::nullopt,
         .snowMillimetres = row.value(QStringLiteral("snow")).isDouble()
                                ? std::optional<double>{row.value(QStringLiteral("snow")).toDouble()}
                                : std::nullopt});
  }
  cached.sunsetUtc = QDateTime::fromString(root.value(QStringLiteral("sunset")).toString(), Qt::ISODate);
  cached.todayPrecipitationKind = root.value(QStringLiteral("precipitationKind")).toString();
  if (cached.todayPrecipitationKind.isEmpty())
    cached.todayPrecipitationKind = QStringLiteral("none");
  const auto air = root.value(QStringLiteral("airQuality")).toObject();
  if (!air.isEmpty())
    cached.airQuality = AirQuality{.index = air.value(QStringLiteral("index")).toInt(),
                                   .category = air.value(QStringLiteral("category")).toString()};
  snapshot_ = cached;
  updateTodayPrecipitationProbability();
  scheduleLocalHourBoundary();
  hourlyModel_.replace(cached.hourly, cached.timezoneOffsetSeconds);
  dailyModel_.replace(cached.daily, cached.timezoneOffsetSeconds);
  updateNextSolarEvent();
  stale_ = cached.fetchedUtc.secsTo(QDateTime::currentDateTimeUtc()) > kCacheStaleSeconds;
  state_ = QStringLiteral("ready");
}

// NOLINTEND(readability-braces-around-statements,cppcoreguidelines-narrowing-conversions,performance-unnecessary-value-param)
}  // namespace dashboard::weather
