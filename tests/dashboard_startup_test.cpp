#include <QProcess>
#include <QtTest>

class DashboardStartupTest : public QObject {
  Q_OBJECT

 private slots:
  void initializesQmlAndKeepsRunning();
};

void DashboardStartupTest::initializesQmlAndKeepsRunning() {  // NOLINT(readability-convert-member-functions-to-static)
  QProcess dashboard;
  auto environment = QProcessEnvironment::systemEnvironment();
  environment.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
  environment.insert(QStringLiteral("QT_FORCE_STDERR_LOGGING"), QStringLiteral("1"));
  dashboard.setProcessEnvironment(environment);
  dashboard.setProgram(QStringLiteral(DASHBOARD_EXECUTABLE));
  dashboard.start();

  QVERIFY2(dashboard.waitForStarted(), qPrintable(dashboard.errorString()));
  QVERIFY2(!dashboard.waitForFinished(1000), qPrintable(QString::fromUtf8(dashboard.readAllStandardError())));

  dashboard.terminate();
  const bool terminated = dashboard.waitForFinished(3000);
  if (!terminated) {
    dashboard.kill();
    dashboard.waitForFinished(3000);
  }

  QVERIFY2(terminated, "dashboard did not terminate promptly");
}

QTEST_GUILESS_MAIN(DashboardStartupTest)

#include "dashboard_startup_test.moc"
