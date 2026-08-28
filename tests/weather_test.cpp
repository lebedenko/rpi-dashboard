#include "weather/weather_config.h"
#include "weather/weather_models.h"
#include "weather/weather_provider.h"
#include "weather_icon_provider.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTest>

using namespace dashboard::weather;
// NOLINTBEGIN(readability-convert-member-functions-to-static,readability-identifier-naming)

class WeatherTest final : public QObject {
  Q_OBJECT

 private slots:
  void parsesLocationModes();
  void rejectsInvalidConfiguration_data();
  void rejectsInvalidConfiguration();
  void credentialFileTakesPrecedence();
  void parsesProviderFixtures();
  void truncatesForecastModels();
  void validatesAndRecolorsIcons();
};

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
}

void WeatherTest::truncatesForecastModels() {
  QVector<HourlyForecast> hourly(12);
  HourlyForecastModel hourlyModel;
  hourlyModel.replace(hourly, 0);
  QCOMPARE(hourlyModel.rowCount(), 8);
  QVector<DailyForecast> daily(8);
  DailyForecastModel dailyModel;
  dailyModel.replace(daily, 0);
  QCOMPARE(dailyModel.rowCount(), 5);
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

QTEST_MAIN(WeatherTest)
#include "weather_test.moc"
// NOLINTEND(readability-convert-member-functions-to-static,readability-identifier-naming)
