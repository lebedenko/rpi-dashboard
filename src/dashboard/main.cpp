#include "dashboard_config.h"
#include "device_model.h"
#include "projects/github_credentials.h"
#include "projects/projects_service.h"
#include "sysinfo/linux_sys_info_collector.h"
#include "sysinfo/sys_info_service.h"
#include "sysmetrics/linux_sys_metrics_collector.h"
#include "sysmetrics/sys_metrics_service.h"
#include "telemetry/remote_device_registry.h"
#include "telemetry/udp_telemetry_receiver.h"
#include "weather/weather_config.h"
#include "weather/weather_service.h"
#include "weather_icon_provider.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QVariantMap>

#include <cstdlib>
#include <memory>

int main(int argc, char* argv[]) {
  QGuiApplication application(argc, argv);
  QGuiApplication::setApplicationName(QStringLiteral("rpi-dashboard"));
  QGuiApplication::setOrganizationName(QStringLiteral("rpi-dashboard"));

  QCommandLineParser parser;
  parser.setApplicationDescription(QStringLiteral("Standalone Raspberry Pi dashboard"));
  parser.addHelpOption();
  const QCommandLineOption windowedOption(QStringLiteral("windowed"),
                                          QStringLiteral("Open a development window instead of fullscreen."));
  const QCommandLineOption widthOption(QStringLiteral("width"), QStringLiteral("Set the windowed width."),
                                       QStringLiteral("pixels"), QStringLiteral("1480"));
  const QCommandLineOption heightOption(QStringLiteral("height"), QStringLiteral("Set the windowed height."),
                                        QStringLiteral("pixels"), QStringLiteral("320"));
  const QCommandLineOption githubOwnerOption(QStringLiteral("github-owner"), QStringLiteral("GitHub account owner."),
                                             QStringLiteral("login"), QStringLiteral("lebedenko"));
  const QCommandLineOption telemetryAddressOption(QStringLiteral("telemetry-bind-address"),
                                                  QStringLiteral("IPv4 address for remote telemetry."),
                                                  QStringLiteral("address"), QStringLiteral("0.0.0.0"));
  const QCommandLineOption telemetryPortOption(QStringLiteral("telemetry-port"),
                                               QStringLiteral("UDP port for remote telemetry."), QStringLiteral("port"),
                                               QStringLiteral("51337"));
  const QCommandLineOption configOption(QStringLiteral("config"), QStringLiteral("Use dashboard configuration file."),
                                        QStringLiteral("path"));
  parser.addOptions({windowedOption, widthOption, heightOption, githubOwnerOption, telemetryAddressOption,
                     telemetryPortOption, configOption});
  parser.process(application);

  const auto configResult = dashboard::loadDashboardConfig(parser.value(configOption));
  if (!configResult.diagnostic.isEmpty()) {
    qWarning().noquote() << configResult.diagnostic;
  }
  if (configResult.fatal) {
    return EXIT_FAILURE;
  }
  const auto& config = configResult.config;

  bool widthIsValid = false;
  bool heightIsValid = false;
  const int windowWidth =
      parser.isSet(widthOption) ? parser.value(widthOption).toInt(&widthIsValid) : config.window_width;
  const int windowHeight =
      parser.isSet(heightOption) ? parser.value(heightOption).toInt(&heightIsValid) : config.window_height;
  widthIsValid = widthIsValid || !parser.isSet(widthOption);
  heightIsValid = heightIsValid || !parser.isSet(heightOption);
  if (!widthIsValid || windowWidth <= 0) {
    parser.showHelp(EXIT_FAILURE);
  }
  if (!heightIsValid || windowHeight <= 0) {
    parser.showHelp(EXIT_FAILURE);
  }
  bool portIsValid = false;
  const int telemetryPort =
      parser.isSet(telemetryPortOption) ? parser.value(telemetryPortOption).toInt(&portIsValid) : config.telemetry_port;
  portIsValid = portIsValid || !parser.isSet(telemetryPortOption);
  const QString telemetryAddressText =
      parser.isSet(telemetryAddressOption) ? parser.value(telemetryAddressOption) : config.telemetry_bind_address;
  const QHostAddress telemetryAddress(telemetryAddressText);
  if (!portIsValid || telemetryPort < 1 || telemetryPort > 65535 ||
      telemetryAddress.protocol() != QAbstractSocket::IPv4Protocol) {
    parser.showHelp(EXIT_FAILURE);
  }

  dashboard::sysinfo::SysInfoService sys_info_service(std::make_shared<dashboard::sysinfo::LinuxSysInfoCollector>());
  dashboard::sysmetrics::SysMetricsService sys_metrics_service(
      std::make_shared<dashboard::sysmetrics::LinuxSysMetricsCollector>());
  const QByteArray githubTokenFile =
      qEnvironmentVariableIsSet("GITHUB_TOKEN_FILE") ? qgetenv("GITHUB_TOKEN_FILE") : config.github_token_file;
  const auto credential = dashboard::projects::loadGitHubCredential(githubTokenFile, qgetenv("GITHUB_TOKEN"));
  if (!credential.diagnostic.isEmpty()) {
    qWarning().noquote() << credential.diagnostic;
  }
  const QString githubOwner = parser.isSet(githubOwnerOption) ? parser.value(githubOwnerOption) : config.github_owner;
  dashboard::projects::ProjectsService projects_service(githubOwner, credential.token);
  const QByteArray openWeatherKeyFile = qEnvironmentVariableIsSet("OPENWEATHER_API_KEY_FILE")
                                            ? qgetenv("OPENWEATHER_API_KEY_FILE")
                                            : config.open_weather_api_key_file;
  const QByteArray ipGeolocationKeyFile = qEnvironmentVariableIsSet("IPGEOLOCATION_API_KEY_FILE")
                                              ? qgetenv("IPGEOLOCATION_API_KEY_FILE")
                                              : config.ip_geolocation_api_key_file;
  const auto openweather_credential = dashboard::weather::loadCredential(
      openWeatherKeyFile, qgetenv("OPENWEATHER_API_KEY"), QStringLiteral("OpenWeather"));
  const auto ipgeolocation_credential = dashboard::weather::loadCredential(
      ipGeolocationKeyFile, qgetenv("IPGEOLOCATION_API_KEY"), QStringLiteral("IP geolocation"));
  dashboard::weather::WeatherService weather_service(config.weather, openweather_credential.value,
                                                     ipgeolocation_credential.value);
  dashboard::telemetry::RemoteDeviceRegistry remote_devices;
  dashboard::DeviceModel device_model(&sys_info_service, &sys_metrics_service, &remote_devices);
  dashboard::telemetry::UdpTelemetryReceiver telemetry_receiver(&remote_devices);
  if (!telemetry_receiver.bind(telemetryAddress, static_cast<quint16>(telemetryPort))) {
    qWarning().noquote() << telemetry_receiver.diagnostic();
  }
  QQmlApplicationEngine engine;
  engine.addImageProvider(QStringLiteral("weather"), new dashboard::WeatherIconProvider());
  engine.setInitialProperties({{QStringLiteral("windowed"), parser.isSet(windowedOption) || config.windowed},
                               {QStringLiteral("windowWidth"), windowWidth},
                               {QStringLiteral("windowHeight"), windowHeight},
                               {QStringLiteral("sysInfoService"), QVariant::fromValue(&sys_info_service)},
                               {QStringLiteral("sysMetricsService"), QVariant::fromValue(&sys_metrics_service)},
                               {QStringLiteral("projectsService"), QVariant::fromValue(&projects_service)},
                               {QStringLiteral("weatherService"), QVariant::fromValue(&weather_service)},
                               {QStringLiteral("deviceModel"), QVariant::fromValue(&device_model)}});
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &application, [] { QCoreApplication::exit(EXIT_FAILURE); },
      Qt::QueuedConnection);
  engine.loadFromModule("Rpi.Dashboard", "Main");
  return QGuiApplication::exec();
}
