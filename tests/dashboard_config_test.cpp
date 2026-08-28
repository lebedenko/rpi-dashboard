#include "dashboard_config.h"

#include <QFile>
#include <QTest>

using dashboard::DashboardConfig;
using dashboard::parseDashboardConfig;

// NOLINTBEGIN(readability-convert-member-functions-to-static,readability-identifier-naming)

class DashboardConfigTest final : public QObject {
  Q_OBJECT

 private slots:
  void parsesCompleteConfiguration();
  void shippedTemplateIsCompleteAndValid();
  void rejectsInvalidRuntimeSettings_data();
  void rejectsInvalidRuntimeSettings();
};

void DashboardConfigTest::parsesCompleteConfiguration() {
  const auto config = parseDashboardConfig(R"(
[display]
windowed = true
width = 1280
height = 300
[projects]
github_owner = "octocat"
[telemetry]
bind_address = "127.0.0.1"
port = 50000
[credentials]
github_token_file = "/run/credentials/github"
openweather_api_key_file = "/run/credentials/weather"
ipgeolocation_api_key_file = "/run/credentials/location"
[weather]
provider = "openweather"
refresh_interval_seconds = 900
[weather.location]
city = "Lviv,UA"
)");

  QVERIFY(config.windowed);
  QCOMPARE(config.window_width, 1280);
  QCOMPARE(config.window_height, 300);
  QCOMPARE(config.github_owner, QStringLiteral("octocat"));
  QCOMPARE(config.telemetry_bind_address, QStringLiteral("127.0.0.1"));
  QCOMPARE(config.telemetry_port, 50000);
  QCOMPARE(config.github_token_file, QByteArrayLiteral("/run/credentials/github"));
  QVERIFY(config.weather.has_value());
  QCOMPARE(config.weather->refreshIntervalSeconds, 900);
}

void DashboardConfigTest::shippedTemplateIsCompleteAndValid() {
  QFile file(QStringLiteral(DASHBOARD_CONFIG_SOURCE));
  QVERIFY(file.open(QIODevice::ReadOnly));
  const auto contents = file.readAll();
  const auto config = parseDashboardConfig(contents);
  QVERIFY(config.weather.has_value());
  for (const auto* key : {"windowed", "width", "height", "github_owner", "bind_address", "port", "github_token_file",
                          "openweather_api_key_file", "ipgeolocation_api_key_file", "provider",
                          "refresh_interval_seconds", "automatic_provider", "city", "latitude", "longitude"}) {
    QVERIFY2(contents.contains(key), key);
  }
}

void DashboardConfigTest::rejectsInvalidRuntimeSettings_data() {
  QTest::addColumn<QByteArray>("contents");
  QTest::newRow("width") << QByteArrayLiteral("[display]\nwidth=0\n");
  QTest::newRow("port") << QByteArrayLiteral("[telemetry]\nport=70000\n");
  QTest::newRow("address") << QByteArrayLiteral("[telemetry]\nbind_address='localhost'\n");
  QTest::newRow("owner") << QByteArrayLiteral("[projects]\ngithub_owner=''\n");
}

void DashboardConfigTest::rejectsInvalidRuntimeSettings() {
  QFETCH(QByteArray, contents);
  const auto parse = [&contents] { static_cast<void>(parseDashboardConfig(contents)); };
  QVERIFY_EXCEPTION_THROWN(parse(), std::runtime_error);
}

QTEST_MAIN(DashboardConfigTest)
#include "dashboard_config_test.moc"
// NOLINTEND(readability-convert-member-functions-to-static,readability-identifier-naming)
