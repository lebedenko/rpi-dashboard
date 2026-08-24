#include <QCoreApplication>
#include <QTextStream>

int main(int argc, char* argv[]) {
  QCoreApplication application(argc, argv);
  QCoreApplication::setApplicationName(QStringLiteral("HoloNight Dashboard Agent"));

  QTextStream(stdout) << "holonight-dashboard-agent: telemetry transport is not configured\n";
  return 0;
}

