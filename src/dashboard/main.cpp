#include "sysinfo/linux_sys_info_collector.h"
#include "sysinfo/sys_info_service.h"
#include "sysmetrics/linux_sys_metrics_collector.h"
#include "sysmetrics/sys_metrics_service.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QVariantMap>

#include <cstdlib>
#include <memory>

int main(int argc, char* argv[]) {
  QGuiApplication application(argc, argv);
  QGuiApplication::setApplicationName(QStringLiteral("HoloNight Dashboard"));
  QGuiApplication::setOrganizationName(QStringLiteral("HoloNight"));

  QCommandLineParser parser;
  parser.setApplicationDescription(QStringLiteral("Standalone Raspberry Pi dashboard"));
  parser.addHelpOption();
  const QCommandLineOption windowedOption(QStringLiteral("windowed"),
                                          QStringLiteral("Open a development window instead of fullscreen."));
  const QCommandLineOption widthOption(QStringLiteral("width"), QStringLiteral("Set the windowed width."),
                                       QStringLiteral("pixels"), QStringLiteral("1480"));
  const QCommandLineOption heightOption(QStringLiteral("height"), QStringLiteral("Set the windowed height."),
                                        QStringLiteral("pixels"), QStringLiteral("320"));
  parser.addOptions({windowedOption, widthOption, heightOption});
  parser.process(application);

  bool widthIsValid = false;
  bool heightIsValid = false;
  const int windowWidth = parser.value(widthOption).toInt(&widthIsValid);
  const int windowHeight = parser.value(heightOption).toInt(&heightIsValid);
  if (!widthIsValid || windowWidth <= 0) {
    parser.showHelp(EXIT_FAILURE);
  }
  if (!heightIsValid || windowHeight <= 0) {
    parser.showHelp(EXIT_FAILURE);
  }

  dashboard::sysinfo::SysInfoService sys_info_service(std::make_shared<dashboard::sysinfo::LinuxSysInfoCollector>());
  dashboard::sysmetrics::SysMetricsService sys_metrics_service(
      std::make_shared<dashboard::sysmetrics::LinuxSysMetricsCollector>());
  QQmlApplicationEngine engine;
  engine.setInitialProperties({{QStringLiteral("windowed"), parser.isSet(windowedOption)},
                               {QStringLiteral("windowWidth"), windowWidth},
                               {QStringLiteral("windowHeight"), windowHeight},
                               {QStringLiteral("sysInfoService"), QVariant::fromValue(&sys_info_service)},
                               {QStringLiteral("sysMetricsService"), QVariant::fromValue(&sys_metrics_service)}});
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &application, [] { QCoreApplication::exit(EXIT_FAILURE); },
      Qt::QueuedConnection);
  engine.loadFromModule("HoloNight.Dashboard", "Main");
  return QGuiApplication::exec();
}
