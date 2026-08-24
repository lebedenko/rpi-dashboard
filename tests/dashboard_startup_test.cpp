#include <QFile>
#include <QProcess>
#include <QtTest>

class DashboardStartupTest : public QObject {
  Q_OBJECT

 private slots:
  void sidebarDrawsOnlyInternalSeparator();
  void declaresAndUsesTypographyRoles();
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

void DashboardStartupTest::declaresAndUsesTypographyRoles() {  // NOLINT(readability-convert-member-functions-to-static)
  QFile themeQml(QStringLiteral(DASHBOARD_THEME_QML));
  QVERIFY2(themeQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(themeQml.errorString()));
  const auto themeSource = QString::fromUtf8(themeQml.readAll());

  QVERIFY(themeSource.contains(QStringLiteral("installedFontFamilies.includes(\"Rajdhani\")")));
  QVERIFY(themeSource.contains(QStringLiteral("installedFontFamilies.includes(\"JetBrains Mono\")")));
  QVERIFY(themeSource.contains(QStringLiteral("installedFontFamilies.includes(\"IBM Plex Mono\")")));
  QVERIFY(themeSource.contains(QStringLiteral("return \"monospace\"")));
  QVERIFY(themeSource.contains(QStringLiteral("headingFontWeight: Font.DemiBold")));
  QVERIFY(themeSource.contains(QStringLiteral("informationFontWeight: Font.Medium")));
  QVERIFY(themeSource.contains(QStringLiteral("metricFontWeight: Font.Light")));

  QFile mainQml(QStringLiteral(DASHBOARD_MAIN_QML));
  QVERIFY2(mainQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(mainQml.errorString()));
  const auto mainSource = QString::fromUtf8(mainQml.readAll());
  QCOMPARE(mainSource.count(QStringLiteral("font.family: Theme.sansFontFamily")), 3);
  QCOMPARE(mainSource.count(QStringLiteral("font.weight: Theme.headingFontWeight")), 3);

  QFile pageQml(QStringLiteral(DASHBOARD_PAGE_QML));
  QVERIFY2(pageQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(pageQml.errorString()));
  const auto pageSource = QString::fromUtf8(pageQml.readAll());
  QCOMPARE(pageSource.count(QStringLiteral("font.family: Theme.sansFontFamily")), 2);
  QVERIFY(pageSource.contains(QStringLiteral("font.weight: Theme.headingFontWeight")));
  QVERIFY(pageSource.contains(QStringLiteral("font.weight: Theme.informationFontWeight")));
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
