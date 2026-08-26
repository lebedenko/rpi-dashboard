#include <QCoreApplication>
#include <QTextStream>

int main(int argc, char* argv[]) {
  QCoreApplication application(argc, argv);
  QCoreApplication::setApplicationName(QStringLiteral("rpi-dashboard-agent"));

  QTextStream(stdout) << "rpi-dashboard-agent: telemetry transport is not configured\n";
  return 0;
}
