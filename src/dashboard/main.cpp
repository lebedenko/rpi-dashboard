#include "projects/github_credentials.h"
#include "projects/projects_service.h"
#include "sysinfo/linux_sys_info_collector.h"
#include "sysinfo/sys_info_service.h"
#include "sysmetrics/linux_sys_metrics_collector.h"
#include "sysmetrics/sys_metrics_service.h"

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
  const QCommandLineOption githubOwnerOption(QStringLiteral("github-owner"), QStringLiteral("GitHub account owner."),
                                             QStringLiteral("login"), QStringLiteral("lebedenko"));
  parser.addOptions({windowedOption, widthOption, heightOption, githubOwnerOption});
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
  const auto credential =
      dashboard::projects::loadGitHubCredential(qgetenv("GITHUB_TOKEN_FILE"), qgetenv("GITHUB_TOKEN"));
  if (!credential.diagnostic.isEmpty()) {
    qWarning().noquote() << credential.diagnostic;
  }
  dashboard::projects::ProjectsService projects_service(parser.value(githubOwnerOption), credential.token);
  QQmlApplicationEngine engine;
  engine.setInitialProperties({{QStringLiteral("windowed"), parser.isSet(windowedOption)},
                               {QStringLiteral("windowWidth"), windowWidth},
                               {QStringLiteral("windowHeight"), windowHeight},
                               {QStringLiteral("sysInfoService"), QVariant::fromValue(&sys_info_service)},
                               {QStringLiteral("sysMetricsService"), QVariant::fromValue(&sys_metrics_service)},
                               {QStringLiteral("projectsService"), QVariant::fromValue(&projects_service)}});
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &application, [] { QCoreApplication::exit(EXIT_FAILURE); },
      Qt::QueuedConnection);
  engine.loadFromModule("HoloNight.Dashboard", "Main");
  return QGuiApplication::exec();
}
