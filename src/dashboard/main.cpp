#include <QGuiApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QQmlApplicationEngine>
#include <QVariantMap>

#include <cstdlib>

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

  QQmlApplicationEngine engine;
  engine.setInitialProperties({{QStringLiteral("windowed"), parser.isSet(windowedOption)},
                               {QStringLiteral("windowWidth"), windowWidth},
                               {QStringLiteral("windowHeight"), windowHeight}});
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &application, [] { QCoreApplication::exit(EXIT_FAILURE); },
      Qt::QueuedConnection);
  engine.loadFromModule("HoloNight.Dashboard", "Main");
  return QGuiApplication::exec();
}
