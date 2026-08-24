#include <QFile>
#include <QProcess>
#include <QtTest>

class DashboardStartupTest : public QObject {
  Q_OBJECT

 private slots:
  void sidebarDrawsOnlyInternalSeparator();
  void initializesQmlAndKeepsRunning();
};

void DashboardStartupTest::sidebarDrawsOnlyInternalSeparator() {  // NOLINT(readability-convert-member-functions-to-static)
  QFile mainQml(QStringLiteral(DASHBOARD_MAIN_QML));
  QVERIFY2(mainQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(mainQml.errorString()));
  const auto source = QString::fromUtf8(mainQml.readAll());

  const auto sidebarStart = source.indexOf(QStringLiteral("id: sidebarSurface"));
  const auto contentStart = source.indexOf(QStringLiteral("ColumnLayout {"), sidebarStart);
  QVERIFY(sidebarStart >= 0);
  QVERIFY(contentStart > sidebarStart);

  const auto sidebarSurface = source.sliced(sidebarStart, contentStart - sidebarStart);
  QVERIFY2(!sidebarSurface.contains(QStringLiteral("border.")), "sidebar surface must not border physical edges");
  QVERIFY(source.contains(QStringLiteral("id: sidebarSeparator")));
  QVERIFY(source.contains(QStringLiteral("anchors.right: parent.right")));
}

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
