#include "weather/weather_config.h"
#include "weather/weather_models.h"
#include "weather/weather_provider.h"
#include "weather/weather_service.h"
#include "weather_icon_provider.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QTimeZone>
#include <QTimer>

using namespace dashboard::weather;
// NOLINTBEGIN(readability-convert-member-functions-to-static,readability-identifier-naming)

namespace {

class FakeWeatherProvider final : public WeatherProvider {
 public:
  using WeatherProvider::WeatherProvider;
  void request(const Location& location) override {
    ++requests;
    lastLocation = location;
  }
  void succeed(const Location& location, const QString& city = QStringLiteral("Lviv")) {
    Snapshot snapshot{
        .provider = QStringLiteral("openweather"), .location = location, .fetchedUtc = QDateTime::currentDateTimeUtc()};
    snapshot.location.city = city;
    snapshot.current.condition = QStringLiteral("Clear");
    emit forecastReady(snapshot);
  }
  void succeed(const Snapshot& snapshot) { emit forecastReady(snapshot); }
  void fail(const QString& diagnostic, bool authentication = false, int retryAfter = 0) {
    emit failed(diagnostic, authentication, retryAfter);
  }
  void failAirQuality(const QString& diagnostic) { emit airQualityFailed(diagnostic); }

  int requests{};
  Location lastLocation;
};

class FakeGeocodingProvider final : public GeocodingProvider {
 public:
  using GeocodingProvider::GeocodingProvider;
  void resolve(const QString& city) override {
    ++requests;
    lastCity = city;
  }
  void succeed(const Location& location) { emit resolved(location); }
  void fail(const QString& diagnostic, bool authentication = false, int retryAfter = 0) {
    emit failed(diagnostic, authentication, retryAfter);
  }

  int requests{};
  QString lastCity;
};

class FakeAutomaticLocationProvider final : public AutomaticLocationProvider {
 public:
  using AutomaticLocationProvider::AutomaticLocationProvider;
  void resolve() override { ++requests; }
  void succeed(const Location& location) { emit resolved(location); }
  void fail(const QString& diagnostic, bool authentication = false, int retryAfter = 0) {
    emit failed(diagnostic, authentication, retryAfter);
  }

  int requests{};
};

WeatherConfig configuration(LocationMode mode) {
  WeatherConfig config;
  config.locationMode = mode;
  config.refreshIntervalSeconds = 60;
  config.city = QStringLiteral("Lviv,UA");
  return config;
}

void fireRetryTimer(WeatherService& service) {
  auto* timer = service.findChild<QTimer*>(QStringLiteral("weatherRefreshTimer"));
  QVERIFY(timer);
  timer->stop();
  QVERIFY(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection));
}

QStringList& warningMessages() {
  static QStringList messages;
  return messages;
}
void captureWarnings(QtMsgType type, const QMessageLogContext& context, const QString& message) {
  Q_UNUSED(context);
  if (type == QtWarningMsg) {
    warningMessages().push_back(message);
  }
}

}  // namespace

class WeatherTest final : public QObject {
  Q_OBJECT

 private slots:
  void init();
  void parsesLocationModes();
  void rejectsInvalidConfiguration_data();
  void rejectsInvalidConfiguration();
  void credentialFileTakesPrecedence();
  void parsesProviderFixtures();
  void selectsNextSolarEventAcrossDayBoundary();
  void selectsSolarEventsBeforeDawnAndDuringDay();
  void fallsBackWhenSolarEventsAreUnavailable();
  void formatsNextSolarEventAtForecastOffset();
  void switchesSolarEventAtBoundary();
  void rejectsIncompleteCache();
  void derivesRemainingDayPrecipitationAtLocationOffset();
  void returnsZeroWithoutRemainingHourlyForecasts();
  void recomputesPrecipitationAtLocalHourBoundary();
  void cachesCompleteHourlyForecast();
  void truncatesForecastModels();
  void validatesAndRecolorsIcons();
  void automaticLocationRecoversOnTimedRetry();
  void cityLocationRecoversOnTimedAndManualRetry();
  void manualRefreshRetriesAuthenticationFailure();
  void forecastRetryDoesNotResolveLocationAgain();
  void operationsDoNotOverlapAndZeroCoordinatesAreResolved();
  void cachedDataSurvivesSanitizedRuntimeFailures();
  void rejectsParentedInjectedProvider();
};

void WeatherTest::rejectsParentedInjectedProvider() {
  QObject owner;
  auto weather = std::make_unique<FakeWeatherProvider>();
  weather->setParent(&owner);
  QVERIFY_EXCEPTION_THROWN(
      WeatherService(configuration(LocationMode::Coordinates), std::move(weather),
                     std::make_unique<FakeGeocodingProvider>(), std::make_unique<FakeAutomaticLocationProvider>()),
      std::invalid_argument);
}

void WeatherTest::init() {
  QStandardPaths::setTestModeEnabled(true);
  const auto cache =
      QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath(QStringLiteral("weather.json"));
  QFile::remove(cache);
}

void WeatherTest::parsesLocationModes() {
  auto coordinates = parseWeatherConfig(
      "[weather]\nprovider='openweather'\nrefresh_interval_seconds=600\n"
      "[weather.location]\nlatitude=49.8\nlongitude=24.0\n");
  QCOMPARE(coordinates.locationMode, LocationMode::Coordinates);
  auto city = parseWeatherConfig(
      "[weather]\nprovider='openweather'\nrefresh_interval_seconds=60\n"
      "[weather.location]\ncity='Lviv,UA'\n");
  QCOMPARE(city.locationMode, LocationMode::City);
  auto automatic = parseWeatherConfig(
      "[weather]\nprovider='openweather'\nrefresh_interval_seconds=600\n"
      "[weather.location]\nautomatic_provider='ipgeolocation'\n");
  QCOMPARE(automatic.locationMode, LocationMode::Automatic);
}

void WeatherTest::rejectsInvalidConfiguration_data() {
  QTest::addColumn<QByteArray>("contents");
  QTest::newRow("partial coordinates") << QByteArray(
      "[weather]\nprovider='openweather'\nrefresh_interval_seconds=600\n[weather.location]\nlatitude=1\n");
  QTest::newRow("conflict") << QByteArray(
      "[weather]\nprovider='openweather'\nrefresh_interval_seconds=600\n[weather.location]\nlatitude=1\nlongitude="
      "2\ncity='X'\n");
  QTest::newRow("range") << QByteArray(
      "[weather]\nprovider='openweather'\nrefresh_interval_seconds=600\n[weather.location]\nlatitude=91\nlongitude="
      "2\n");
  QTest::newRow("interval") << QByteArray(
      "[weather]\nprovider='openweather'\nrefresh_interval_seconds=10\n[weather.location]\ncity='X'\n");
  QTest::newRow("provider") << QByteArray(
      "[weather]\nprovider='other'\nrefresh_interval_seconds=600\n[weather.location]\ncity='X'\n");
}

void WeatherTest::rejectsInvalidConfiguration() {
  QFETCH(QByteArray, contents);
  const auto parse = [&contents] { static_cast<void>(parseWeatherConfig(contents)); };
  QVERIFY_EXCEPTION_THROWN(parse(), std::runtime_error);
}

void WeatherTest::credentialFileTakesPrecedence() {
  QTemporaryDir directory;
  QFile file(directory.filePath(QStringLiteral("credential")));
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write("file-secret\n");
  file.close();
  const auto result = loadCredential(QFile::encodeName(file.fileName()), QByteArrayLiteral("environment-secret"),
                                     QStringLiteral("Weather"));
  QCOMPARE(result.value, QByteArrayLiteral("file-secret"));
  QVERIFY(result.diagnostic.isEmpty());
}

void WeatherTest::parsesProviderFixtures() {
  const auto location = OpenWeatherGeocodingProvider::parseGeocoding(
      QJsonDocument::fromJson(R"([{"name":"Lviv","lat":49.84,"lon":24.03,"country":"UA"}])"));
  QCOMPARE(location.city, QStringLiteral("Lviv"));
  const auto aqi = OpenWeatherProvider::parseAirQuality(
      QJsonDocument::fromJson(R"({"list":[{"main":{"aqi":2},"components":{"pm2_5":7.5}}]})"));
  QCOMPARE(aqi.index, 2);
  QCOMPARE(aqi.category, QStringLiteral("Fair"));
  const auto automatic = IpGeolocationProvider::parseLocation(
      QJsonDocument::fromJson(R"({"city":"Lviv","country_code2":"UA","latitude":"49.84","longitude":"24.03"})"));
  QCOMPARE(automatic.country, QStringLiteral("UA"));

  const auto forecast = OpenWeatherProvider::parseOneCall(QJsonDocument::fromJson(R"({
        "timezone":"Europe/Kyiv","timezone_offset":10800,
        "current":{"dt":1700000000,"temp":20,"feels_like":19,"humidity":60,"pressure":1012,
                   "wind_speed":2,"wind_deg":180,"weather":[{"description":"clear","icon":"01d"}]},
        "hourly":[],
        "daily":[{"dt":1700000000,"sunrise":1700001000,"sunset":1700040000,"pop":0.1,
                  "rain":1.25,"snow":0.5,
                  "temp":{"min":12,"max":22,"morn":12,"day":22,"eve":18,"night":16},
                  "weather":[{"icon":"01d"}]}]
      })"),
                                                          location);
  QCOMPARE(forecast.daily.first().sunriseUtc, QDateTime::fromSecsSinceEpoch(1700001000, QTimeZone::UTC));
  QCOMPARE(forecast.daily.first().sunsetUtc, QDateTime::fromSecsSinceEpoch(1700040000, QTimeZone::UTC));
  QCOMPARE(forecast.daily.first().averageCelsius, std::optional<double>{17.0});
  QCOMPARE(forecast.daily.first().rainMillimetres, std::optional<double>{1.25});
  QCOMPARE(forecast.daily.first().snowMillimetres, std::optional<double>{0.5});
  QCOMPARE(forecast.todayPrecipitationKind, QStringLiteral("mixed"));
  QCOMPARE(forecast.todayPrecipitationProbabilityPercent, 0.0);
}

void WeatherTest::selectsNextSolarEventAcrossDayBoundary() {
  auto weather = std::make_unique<FakeWeatherProvider>();
  auto* weatherFake = weather.get();
  auto geocoding = std::make_unique<FakeGeocodingProvider>();
  auto automatic = std::make_unique<FakeAutomaticLocationProvider>();
  auto config = configuration(LocationMode::Coordinates);
  const auto now = QDateTime::currentDateTimeUtc();
  WeatherService service(config, std::move(weather), std::move(geocoding), std::move(automatic));
  Snapshot snapshot{.provider = QStringLiteral("openweather"),
                    .location = weatherFake->lastLocation,
                    .fetchedUtc = now,
                    .current = {.iconCode = QStringLiteral("01n")}};
  snapshot.daily = {{.sunriseUtc = now.addSecs(-12 * 3600), .sunsetUtc = now.addSecs(-3600)},
                    {.sunriseUtc = now.addSecs(8 * 3600), .sunsetUtc = now.addSecs(20 * 3600)}};
  weatherFake->succeed(snapshot);

  QCOMPARE(service.nextSolarEventKind(), QStringLiteral("sunrise"));
  QCOMPARE(service.localNextSolarEventTime(), QLocale().toString(now.addSecs(8 * 3600).time(), QLocale::ShortFormat));
}

void WeatherTest::selectsSolarEventsBeforeDawnAndDuringDay() {
  auto weather = std::make_unique<FakeWeatherProvider>();
  auto* weatherFake = weather.get();
  auto geocoding = std::make_unique<FakeGeocodingProvider>();
  auto automatic = std::make_unique<FakeAutomaticLocationProvider>();
  WeatherService service(configuration(LocationMode::Coordinates), std::move(weather), std::move(geocoding),
                         std::move(automatic));
  const auto now = QDateTime::currentDateTimeUtc();
  Snapshot snapshot{.provider = QStringLiteral("openweather"),
                    .location = weatherFake->lastLocation,
                    .fetchedUtc = now,
                    .current = {.iconCode = QStringLiteral("01n")},
                    .daily = {{.sunriseUtc = now.addSecs(3600), .sunsetUtc = now.addSecs(12 * 3600)}}};
  weatherFake->succeed(snapshot);
  QCOMPARE(service.nextSolarEventKind(), QStringLiteral("sunrise"));

  snapshot.current.iconCode = QStringLiteral("01d");
  snapshot.daily.first().sunriseUtc = now.addSecs(-3600);
  weatherFake->succeed(snapshot);
  QCOMPARE(service.nextSolarEventKind(), QStringLiteral("sunset"));
}

void WeatherTest::fallsBackWhenSolarEventsAreUnavailable() {
  auto weather = std::make_unique<FakeWeatherProvider>();
  auto* weatherFake = weather.get();
  auto geocoding = std::make_unique<FakeGeocodingProvider>();
  auto automatic = std::make_unique<FakeAutomaticLocationProvider>();
  WeatherService service(configuration(LocationMode::Coordinates), std::move(weather), std::move(geocoding),
                         std::move(automatic));
  Snapshot snapshot{.provider = QStringLiteral("openweather"),
                    .location = weatherFake->lastLocation,
                    .fetchedUtc = QDateTime::currentDateTimeUtc(),
                    .current = {.iconCode = QStringLiteral("02n")}};
  weatherFake->succeed(snapshot);
  QCOMPARE(service.nextSolarEventKind(), QStringLiteral("sunrise"));
  QVERIFY(service.localNextSolarEventTime().isEmpty());
}

void WeatherTest::formatsNextSolarEventAtForecastOffset() {
  constexpr int kForecastOffsetSeconds = (5 * 3600) + (30 * 60);
  auto weather = std::make_unique<FakeWeatherProvider>();
  auto* weatherFake = weather.get();
  auto geocoding = std::make_unique<FakeGeocodingProvider>();
  auto automatic = std::make_unique<FakeAutomaticLocationProvider>();
  WeatherService service(configuration(LocationMode::Coordinates), std::move(weather), std::move(geocoding),
                         std::move(automatic));
  const auto event = QDateTime::currentDateTimeUtc().addSecs(3600);
  Snapshot snapshot{.provider = QStringLiteral("openweather"),
                    .location = weatherFake->lastLocation,
                    .timezoneOffsetSeconds = kForecastOffsetSeconds,
                    .fetchedUtc = QDateTime::currentDateTimeUtc(),
                    .daily = {{.sunriseUtc = event}}};
  weatherFake->succeed(snapshot);
  QCOMPARE(service.localNextSolarEventTime(),
           QLocale().toString(event.addSecs(kForecastOffsetSeconds).time(), QLocale::ShortFormat));
}

void WeatherTest::switchesSolarEventAtBoundary() {
  auto weather = std::make_unique<FakeWeatherProvider>();
  auto* weatherFake = weather.get();
  auto geocoding = std::make_unique<FakeGeocodingProvider>();
  auto automatic = std::make_unique<FakeAutomaticLocationProvider>();
  WeatherService service(configuration(LocationMode::Coordinates), std::move(weather), std::move(geocoding),
                         std::move(automatic));
  const auto now = QDateTime::currentDateTimeUtc();
  Snapshot snapshot{.provider = QStringLiteral("openweather"),
                    .location = weatherFake->lastLocation,
                    .fetchedUtc = now,
                    .daily = {{.sunriseUtc = now.addSecs(1), .sunsetUtc = now.addSecs(3600)}}};
  weatherFake->succeed(snapshot);
  QCOMPARE(service.nextSolarEventKind(), QStringLiteral("sunrise"));
  QTRY_COMPARE_WITH_TIMEOUT(service.nextSolarEventKind(), QStringLiteral("sunset"), 2500);
}

void WeatherTest::rejectsIncompleteCache() {
  QTemporaryDir cacheRoot;
  QVERIFY(cacheRoot.isValid());
  const auto previousCacheHome = qgetenv("XDG_CACHE_HOME");
  qputenv("XDG_CACHE_HOME", QFile::encodeName(cacheRoot.path()));
  QStandardPaths::setTestModeEnabled(false);
  auto config = configuration(LocationMode::Coordinates);
  const auto cache =
      QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath(QStringLiteral("weather.json"));
  QDir().mkpath(QFileInfo(cache).absolutePath());
  QFile file(cache);
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write(QJsonDocument(
                 QJsonObject{{QStringLiteral("schemaVersion"), 1},
                             {QStringLiteral("provider"), QStringLiteral("openweather")},
                             {QStringLiteral("location"),
                              QJsonObject{{QStringLiteral("latitude"), 0.0}, {QStringLiteral("longitude"), 0.0}}},
                             {QStringLiteral("timezoneOffset"), 0},
                             {QStringLiteral("fetched"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
                             {QStringLiteral("rainProbability"), 88.0},
                             {QStringLiteral("current"), QJsonObject{{QStringLiteral("icon"), QStringLiteral("01d")}}},
                             {QStringLiteral("hourly"), QJsonArray{}},
                             {QStringLiteral("daily"), QJsonArray{}}})
                 .toJson(QJsonDocument::Compact));
  file.close();
  auto reloadWeather = std::make_unique<FakeWeatherProvider>();
  auto reloadGeocoding = std::make_unique<FakeGeocodingProvider>();
  auto reloadAutomatic = std::make_unique<FakeAutomaticLocationProvider>();
  WeatherService reloaded(config, std::move(reloadWeather), std::move(reloadGeocoding), std::move(reloadAutomatic));
  QCOMPARE(reloaded.state(), QStringLiteral("loading"));
  QVERIFY(reloaded.nextSolarEventKind().isEmpty());
  QVERIFY(reloaded.localNextSolarEventTime().isEmpty());
  QCOMPARE(reloaded.todayPrecipitationProbabilityPercent(), 0.0);
  QCOMPARE(reloaded.todayRainProbabilityPercent(), 0.0);
  QStandardPaths::setTestModeEnabled(true);
  qputenv("XDG_CACHE_HOME", previousCacheHome);
}

void WeatherTest::derivesRemainingDayPrecipitationAtLocationOffset() {
  QDateTime now(QDate(2026, 8, 31), QTime(22, 35), QTimeZone::UTC);
  auto weather = std::make_unique<FakeWeatherProvider>();
  auto* weatherFake = weather.get();
  auto geocoding = std::make_unique<FakeGeocodingProvider>();
  auto automatic = std::make_unique<FakeAutomaticLocationProvider>();
  WeatherService service(configuration(LocationMode::Coordinates), std::move(weather), std::move(geocoding),
                         std::move(automatic), nullptr, [&now] { return now; });
  Snapshot snapshot{.provider = QStringLiteral("openweather"),
                    .location = weatherFake->lastLocation,
                    .timezoneOffsetSeconds = 2 * 3600,
                    .fetchedUtc = now,
                    .hourly = {{.timestampUtc = QDateTime(QDate(2026, 8, 31), QTime(21, 0), QTimeZone::UTC),
                                .precipitationProbabilityPercent = 99.0},
                               {.timestampUtc = QDateTime(QDate(2026, 8, 31), QTime(22, 0), QTimeZone::UTC),
                                .precipitationProbabilityPercent = 25.0},
                               {.timestampUtc = QDateTime(QDate(2026, 9, 1), QTime(21, 0), QTimeZone::UTC),
                                .precipitationProbabilityPercent = 70.0},
                               {.timestampUtc = QDateTime(QDate(2026, 9, 1), QTime(22, 0), QTimeZone::UTC),
                                .precipitationProbabilityPercent = 95.0},
                               {.precipitationProbabilityPercent = 100.0}}};
  weatherFake->succeed(snapshot);
  QCOMPARE(service.todayPrecipitationProbabilityPercent(), 70.0);
  QCOMPARE(service.todayRainProbabilityPercent(), 70.0);
}

void WeatherTest::returnsZeroWithoutRemainingHourlyForecasts() {
  QDateTime now(QDate(2026, 9, 1), QTime(12, 30), QTimeZone::UTC);
  auto weather = std::make_unique<FakeWeatherProvider>();
  auto* weatherFake = weather.get();
  auto geocoding = std::make_unique<FakeGeocodingProvider>();
  auto automatic = std::make_unique<FakeAutomaticLocationProvider>();
  WeatherService service(configuration(LocationMode::Coordinates), std::move(weather), std::move(geocoding),
                         std::move(automatic), nullptr, [&now] { return now; });
  Snapshot snapshot{.provider = QStringLiteral("openweather"),
                    .location = weatherFake->lastLocation,
                    .fetchedUtc = now,
                    .hourly = {{.timestampUtc = now.addSecs(-3600), .precipitationProbabilityPercent = 80.0},
                               {.timestampUtc = now.addDays(1), .precipitationProbabilityPercent = 90.0}},
                    .todayRainProbabilityPercent = 88.0};
  weatherFake->succeed(snapshot);
  QCOMPARE(service.todayPrecipitationProbabilityPercent(), 0.0);
  QCOMPARE(service.todayRainProbabilityPercent(), 0.0);
}

void WeatherTest::recomputesPrecipitationAtLocalHourBoundary() {
  QDateTime now(QDate(2026, 9, 1), QTime(12, 30), QTimeZone::UTC);
  auto weather = std::make_unique<FakeWeatherProvider>();
  auto* weatherFake = weather.get();
  auto geocoding = std::make_unique<FakeGeocodingProvider>();
  auto automatic = std::make_unique<FakeAutomaticLocationProvider>();
  WeatherService service(configuration(LocationMode::Coordinates), std::move(weather), std::move(geocoding),
                         std::move(automatic), nullptr, [&now] { return now; });
  Snapshot snapshot{.provider = QStringLiteral("openweather"),
                    .location = weatherFake->lastLocation,
                    .fetchedUtc = now,
                    .hourly = {{.timestampUtc = now.addSecs(-30 * 60), .precipitationProbabilityPercent = 90.0},
                               {.timestampUtc = now.addSecs(30 * 60), .precipitationProbabilityPercent = 20.0}}};
  weatherFake->succeed(snapshot);
  QCOMPARE(service.todayPrecipitationProbabilityPercent(), 90.0);
  QSignalSpy changed(&service, &WeatherService::changed);
  now = now.addSecs(3600);
  auto* timer = service.findChild<QTimer*>(QStringLiteral("weatherLocalHourTimer"));
  QVERIFY(timer);
  QVERIFY(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection));
  QCOMPARE(service.todayPrecipitationProbabilityPercent(), 20.0);
  QCOMPARE(changed.count(), 1);
}

void WeatherTest::cachesCompleteHourlyForecast() {
  QTemporaryDir cacheRoot;
  QVERIFY(cacheRoot.isValid());
  const auto previousCacheHome = qgetenv("XDG_CACHE_HOME");
  qputenv("XDG_CACHE_HOME", QFile::encodeName(cacheRoot.path()));
  QStandardPaths::setTestModeEnabled(false);
  QDateTime now(QDate(2026, 9, 1), QTime(10, 15), QTimeZone::UTC);
  auto weather = std::make_unique<FakeWeatherProvider>();
  auto* weatherFake = weather.get();
  auto geocoding = std::make_unique<FakeGeocodingProvider>();
  auto automatic = std::make_unique<FakeAutomaticLocationProvider>();
  const auto config = configuration(LocationMode::Coordinates);
  {
    WeatherService service(config, std::move(weather), std::move(geocoding), std::move(automatic), nullptr,
                           [&now] { return now; });
    Snapshot snapshot{
        .provider = QStringLiteral("openweather"), .location = weatherFake->lastLocation, .fetchedUtc = now};
    for (int hour = 0; hour < 12; ++hour) {
      snapshot.hourly.push_back(
          {.timestampUtc = now.addSecs(hour * 3600), .precipitationProbabilityPercent = hour == 10 ? 85.0 : 5.0});
    }
    weatherFake->succeed(snapshot);
  }
  auto reloadWeather = std::make_unique<FakeWeatherProvider>();
  auto reloadGeocoding = std::make_unique<FakeGeocodingProvider>();
  auto reloadAutomatic = std::make_unique<FakeAutomaticLocationProvider>();
  WeatherService reloaded(config, std::move(reloadWeather), std::move(reloadGeocoding), std::move(reloadAutomatic),
                          nullptr, [&now] { return now; });
  QCOMPARE(reloaded.hourlyModel()->rowCount(), 8);
  QCOMPARE(reloaded.todayPrecipitationProbabilityPercent(), 85.0);
  QStandardPaths::setTestModeEnabled(true);
  qputenv("XDG_CACHE_HOME", previousCacheHome);
}

void WeatherTest::truncatesForecastModels() {
  QVector<HourlyForecast> hourly(12);
  for (auto& row : hourly) {
    row.temperatureCelsius = 10.0;
  }
  hourly[0].temperatureCelsius = 10.0;
  hourly[1].temperatureCelsius = 20.0;
  HourlyForecastModel hourlyModel;
  hourlyModel.replace(hourly, 0);
  QCOMPARE(hourlyModel.rowCount(), 8);
  hourly[0].timestampUtc = QDateTime(QDate(2026, 9, 1), QTime(5, 0), QTimeZone::UTC);
  hourlyModel.replace(hourly, 2 * 3600);
  QCOMPARE(hourlyModel.data(hourlyModel.index(0), HourlyForecastModel::LocalHourRole).toString(), QStringLiteral("07"));
  hourly[0].timestampUtc = QDateTime(QDate(2026, 9, 1), QTime(0, 0), QTimeZone::UTC);
  hourly[1].timestampUtc = QDateTime(QDate(2026, 9, 1), QTime(23, 0), QTimeZone::UTC);
  hourlyModel.replace(hourly, 0);
  QCOMPARE(hourlyModel.data(hourlyModel.index(0), HourlyForecastModel::LocalHourRole).toString(), QStringLiteral("00"));
  QCOMPARE(hourlyModel.data(hourlyModel.index(1), HourlyForecastModel::LocalHourRole).toString(), QStringLiteral("23"));
  QCOMPARE(hourlyModel.data(hourlyModel.index(0), HourlyForecastModel::TrendPositionRole).toDouble(), 0.0);
  QCOMPARE(hourlyModel.data(hourlyModel.index(1), HourlyForecastModel::TrendPositionRole).toDouble(), 1.0);
  QVector<HourlyForecast> equal(2);
  equal[0].temperatureCelsius = equal[1].temperatureCelsius = 7.0;
  hourlyModel.replace(equal, 0);
  QCOMPARE(hourlyModel.data(hourlyModel.index(0), HourlyForecastModel::TrendPositionRole).toDouble(), 0.5);
  QVector<DailyForecast> daily(8);
  daily[0].averageCelsius = 17.5;
  DailyForecastModel dailyModel;
  dailyModel.replace(daily, 0);
  QCOMPARE(dailyModel.rowCount(), 5);
  QCOMPARE(dailyModel.data(dailyModel.index(0), DailyForecastModel::AverageRole).toDouble(), 17.5);
  QVERIFY(!dailyModel.data(dailyModel.index(1), DailyForecastModel::AverageRole).isValid());
}

void WeatherTest::validatesAndRecolorsIcons() {
  QCOMPARE(dashboard::WeatherIconProvider::safeIconCode(QStringLiteral("../../secret")), QStringLiteral("03d"));
  const auto svg = dashboard::WeatherIconProvider::recolorStylesheet(
      QByteArrayLiteral("<svg><style id=\"current-color-scheme\">x</style></svg>"), QStringLiteral("#111111"),
      QStringLiteral("#222222"), QStringLiteral("#333333"));
  QVERIFY(svg.contains(".ColorScheme-Text { color: #111111"));
  QDir icons(QStringLiteral(WEATHER_ICON_SOURCE_DIR));
  const auto files = icons.entryList({QStringLiteral("*.svg")}, QDir::Files);
  QCOMPARE(files.size(), 18);
  for (const auto& name : files) {
    QFile file(icons.filePath(name));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto contents = file.readAll();
    QVERIFY(contents.contains("current-color-scheme"));
    QVERIFY(contents.contains("#8b95a5"));
  }
}

void WeatherTest::automaticLocationRecoversOnTimedRetry() {
  auto weather = std::make_unique<FakeWeatherProvider>();
  auto* weatherFake = weather.get();
  auto geocoding = std::make_unique<FakeGeocodingProvider>();
  auto automatic = std::make_unique<FakeAutomaticLocationProvider>();
  auto* automaticFake = automatic.get();
  WeatherService service(configuration(LocationMode::Automatic), std::move(weather), std::move(geocoding),
                         std::move(automatic));

  QCOMPARE(automaticFake->requests, 1);
  automaticFake->fail(QStringLiteral("Automatic location request failed"));
  QCOMPARE(service.state(), QStringLiteral("error"));
  fireRetryTimer(service);
  QCOMPARE(automaticFake->requests, 2);
  automaticFake->succeed({.city = QStringLiteral("Null Island"), .country = QStringLiteral("ZZ")});
  QCOMPARE(weatherFake->requests, 1);
  QCOMPARE(weatherFake->lastLocation.cacheKey(), QStringLiteral("0.0000,0.0000"));
  weatherFake->succeed(weatherFake->lastLocation, QStringLiteral("Null Island"));
  QCOMPARE(service.state(), QStringLiteral("ready"));
}

void WeatherTest::cityLocationRecoversOnTimedAndManualRetry() {
  auto weather = std::make_unique<FakeWeatherProvider>();
  auto* weatherFake = weather.get();
  auto geocoding = std::make_unique<FakeGeocodingProvider>();
  auto* geocodingFake = geocoding.get();
  auto automatic = std::make_unique<FakeAutomaticLocationProvider>();
  WeatherService service(configuration(LocationMode::City), std::move(weather), std::move(geocoding),
                         std::move(automatic));

  geocodingFake->fail(QStringLiteral("Geocoding request failed"));
  fireRetryTimer(service);
  QCOMPARE(geocodingFake->requests, 2);
  geocodingFake->fail(QStringLiteral("Geocoding request failed"));
  service.refresh();
  QCOMPARE(geocodingFake->requests, 3);
  geocodingFake->succeed(
      {.city = QStringLiteral("Lviv"), .country = QStringLiteral("UA"), .latitude = 49.84, .longitude = 24.03});
  QCOMPARE(weatherFake->requests, 1);
}

void WeatherTest::manualRefreshRetriesAuthenticationFailure() {
  auto weather = std::make_unique<FakeWeatherProvider>();
  auto geocoding = std::make_unique<FakeGeocodingProvider>();
  auto automatic = std::make_unique<FakeAutomaticLocationProvider>();
  auto* automaticFake = automatic.get();
  WeatherService service(configuration(LocationMode::Automatic), std::move(weather), std::move(geocoding),
                         std::move(automatic));

  automaticFake->fail(QStringLiteral("Automatic location authentication failed"), true);
  auto* timer = service.findChild<QTimer*>(QStringLiteral("weatherRefreshTimer"));
  QVERIFY(timer);
  QVERIFY(!timer->isActive());
  fireRetryTimer(service);
  QCOMPARE(automaticFake->requests, 1);
  service.refresh();
  QCOMPARE(automaticFake->requests, 2);
}

void WeatherTest::forecastRetryDoesNotResolveLocationAgain() {
  auto weather = std::make_unique<FakeWeatherProvider>();
  auto* weatherFake = weather.get();
  auto geocoding = std::make_unique<FakeGeocodingProvider>();
  auto automatic = std::make_unique<FakeAutomaticLocationProvider>();
  auto* automaticFake = automatic.get();
  WeatherService service(configuration(LocationMode::Automatic), std::move(weather), std::move(geocoding),
                         std::move(automatic));
  const Location location{
      .city = QStringLiteral("Lviv"), .country = QStringLiteral("UA"), .latitude = 49.84, .longitude = 24.03};
  automaticFake->succeed(location);
  weatherFake->fail(QStringLiteral("Weather request failed"));
  fireRetryTimer(service);
  QCOMPARE(automaticFake->requests, 1);
  QCOMPARE(weatherFake->requests, 2);
}

void WeatherTest::operationsDoNotOverlapAndZeroCoordinatesAreResolved() {
  auto weather = std::make_unique<FakeWeatherProvider>();
  auto* weatherFake = weather.get();
  auto geocoding = std::make_unique<FakeGeocodingProvider>();
  auto automatic = std::make_unique<FakeAutomaticLocationProvider>();
  auto* automaticFake = automatic.get();
  WeatherService service(configuration(LocationMode::Automatic), std::move(weather), std::move(geocoding),
                         std::move(automatic));

  service.refresh();
  fireRetryTimer(service);
  QCOMPARE(automaticFake->requests, 1);
  automaticFake->succeed({});
  QCOMPARE(weatherFake->requests, 1);
  service.refresh();
  fireRetryTimer(service);
  QCOMPARE(weatherFake->requests, 1);
}

void WeatherTest::cachedDataSurvivesSanitizedRuntimeFailures() {
  auto config = configuration(LocationMode::Coordinates);
  config.latitude = 49.84;
  config.longitude = 24.03;
  auto weather = std::make_unique<FakeWeatherProvider>();
  auto* weatherFake = weather.get();
  auto geocoding = std::make_unique<FakeGeocodingProvider>();
  auto automatic = std::make_unique<FakeAutomaticLocationProvider>();
  WeatherService service(config, std::move(weather), std::move(geocoding), std::move(automatic));
  weatherFake->succeed(weatherFake->lastLocation);

  warningMessages().clear();
  const auto previousHandler = qInstallMessageHandler(captureWarnings);
  service.refresh();
  weatherFake->fail(QStringLiteral("Weather request failed: https://api.example.test/data?appid=secret&x=1"));
  weatherFake->failAirQuality(QStringLiteral("AQI failed: https://api.example.test/air?apiKey=secret"));
  qInstallMessageHandler(previousHandler);

  QCOMPARE(service.city(), QStringLiteral("Lviv"));
  QVERIFY(service.stale());
  const auto warnings = warningMessages().join(QLatin1Char('\n'));
  QVERIFY(warnings.contains(QStringLiteral("Weather request failed")));
  QVERIFY(warnings.contains(QStringLiteral("AQI failed")));
  QVERIFY(!warnings.contains(QStringLiteral("secret")));
  QVERIFY(!warnings.contains(QStringLiteral("?")));
}

QTEST_MAIN(WeatherTest)
#include "weather_test.moc"
// NOLINTEND(readability-convert-member-functions-to-static,readability-identifier-naming)
