#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char* argv[]) {
  QGuiApplication application(argc, argv);
  QGuiApplication::setApplicationName(QStringLiteral("HoloNight Dashboard"));
  QGuiApplication::setOrganizationName(QStringLiteral("HoloNight"));

  QQmlApplicationEngine engine;
  engine.loadFromModule("HoloNight.Dashboard", "Main");
  return QGuiApplication::exec();
}
