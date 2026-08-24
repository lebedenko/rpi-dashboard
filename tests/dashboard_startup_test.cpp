#include <QFile>
#include <QProcess>
#include <QtTest>

class DashboardStartupTest : public QObject {
  Q_OBJECT

 private slots:
  void sidebarUsesIconOnlySafeLayout();
  void sidebarDrawsOnlyChamferedInternalSeparator();
  void sidebarButtonExposesInteractionStates();
  void sidebarIconsAreTintableSvgResources();
  void declaresAndUsesTypographyRoles();
  void initializesQmlAndKeepsRunning();
};

void DashboardStartupTest::sidebarUsesIconOnlySafeLayout() {  // NOLINT(readability-convert-member-functions-to-static)
  QFile themeQml(QStringLiteral(DASHBOARD_THEME_QML));
  QVERIFY2(themeQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(themeQml.errorString()));
  const auto themeSource = QString::fromUtf8(themeQml.readAll());
  QVERIFY(themeSource.contains(QStringLiteral("sidebarWidth: 88")));
  QVERIFY(themeSource.contains(QStringLiteral("touchTarget: 56")));
  QVERIFY(themeSource.contains(QStringLiteral("displaySafeInset: 11")));

  QFile mainQml(QStringLiteral(DASHBOARD_MAIN_QML));
  QVERIFY2(mainQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(mainQml.errorString()));
  const auto source = QString::fromUtf8(mainQml.readAll());
  QVERIFY(source.contains(QStringLiteral("Layout.preferredWidth: Theme.sidebarWidth")));
  QVERIFY(source.contains(QStringLiteral("anchors.centerIn: parent")));
  QVERIFY(source.contains(QStringLiteral("spacing: Theme.spacingMedium")));
  QCOMPARE(source.count(QStringLiteral("source: \"_SidebarButton.qml\"")), 1);
  QVERIFY(!source.contains(QStringLiteral("qsTr(\"Dashboard\")")));
  QVERIFY(!source.contains(QStringLiteral("text: navigationLoader.modelData.label")));
  QVERIFY(source.contains(QStringLiteral("value: navigationLoader.modelData.label")));
  QVERIFY(source.contains(QStringLiteral("pageStack.currentIndex = navigationLoader.index")));
}

void DashboardStartupTest::sidebarDrawsOnlyChamferedInternalSeparator() {  // NOLINT(readability-convert-member-functions-to-static)
  QFile mainQml(QStringLiteral(DASHBOARD_MAIN_QML));
  QVERIFY2(mainQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(mainQml.errorString()));
  const auto source = QString::fromUtf8(mainQml.readAll());

  const auto sidebarStart = source.indexOf(QStringLiteral("id: sidebarSurface"));
  const auto contentStart = source.indexOf(QStringLiteral("Column {"), sidebarStart);
  QVERIFY(sidebarStart >= 0);
  QVERIFY(contentStart > sidebarStart);

  const auto sidebarSurface = source.sliced(sidebarStart, contentStart - sidebarStart);
  QVERIFY2(!sidebarSurface.contains(QStringLiteral("border.")), "sidebar surface must not border physical edges");
  QVERIFY(source.contains(QStringLiteral("id: sidebarSeparator")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("startX: sidebarSeparator.width - Theme.sidebarChamfer")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("PathLine { x: sidebarSeparator.width; y: Theme.sidebarChamfer }")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("PathLine { x: sidebarSeparator.width; y: sidebarSeparator.height }")));
  QCOMPARE(sidebarSurface.count(QStringLiteral("PathLine")), 2);
}

void DashboardStartupTest::sidebarButtonExposesInteractionStates() {  // NOLINT(readability-convert-member-functions-to-static)
  QFile buttonQml(QStringLiteral(DASHBOARD_SIDEBAR_BUTTON_QML));
  QVERIFY2(buttonQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(buttonQml.errorString()));
  const auto source = QString::fromUtf8(buttonQml.readAll());

  QVERIFY(source.contains(QStringLiteral("display: AbstractButton.IconOnly")));
  QVERIFY(source.contains(QStringLiteral("activeFocusOnTab: true")));
  QVERIFY(source.contains(QStringLiteral("Accessible.name: root.tooltipText")));
  QVERIFY(source.contains(QStringLiteral("root.down ? 0.96 : 1")));
  QVERIFY(source.contains(QStringLiteral("root.hovered")));
  QVERIFY(source.contains(QStringLiteral("root.selected")));
  QVERIFY(source.contains(QStringLiteral("root.activeFocus")));
  QVERIFY(source.contains(QStringLiteral("acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad")));
  QVERIFY(source.contains(QStringLiteral("ToolTip.visible: pointerHover.hovered")));
  QVERIFY(!source.contains(QStringLiteral("ToolTip.visible: root.hovered || root.activeFocus")));
  QVERIFY(source.contains(QStringLiteral("strokeWidth: root.activeFocus ? 2 : root.selected ? 1 : 0")));
  QCOMPARE(source.count(QStringLiteral("PathLine")), 6);
}

void DashboardStartupTest::sidebarIconsAreTintableSvgResources() {  // NOLINT(readability-convert-member-functions-to-static)
  const QStringList iconNames = {QStringLiteral("overview.svg"), QStringLiteral("systems.svg"),
                                 QStringLiteral("projects.svg"), QStringLiteral("weather.svg")};
  for (const auto &iconName : iconNames) {
    QFile icon(QStringLiteral(DASHBOARD_ICON_DIRECTORY) + QLatin1Char('/') + iconName);
    QVERIFY2(icon.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(icon.errorString()));
    const auto source = QString::fromUtf8(icon.readAll());
    QVERIFY2(source.contains(QStringLiteral("viewBox=\"0 0 24 24\"")), qPrintable(iconName));
    QVERIFY2(source.contains(QStringLiteral("stroke-width=\"2\"")), qPrintable(iconName));
  }

  QFile mainQml(QStringLiteral(DASHBOARD_MAIN_QML));
  QVERIFY2(mainQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(mainQml.errorString()));
  const auto mainSource = QString::fromUtf8(mainQml.readAll());
  QCOMPARE(mainSource.count(QStringLiteral("\"icon\": Qt.resolvedUrl(\"icons/")), 4);
  QVERIFY(mainSource.contains(QStringLiteral("value: navigationLoader.modelData.icon")));
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
  QCOMPARE(mainSource.count(QStringLiteral("font.family: Theme.sansFontFamily")), 0);
  QCOMPARE(mainSource.count(QStringLiteral("font.weight: Theme.headingFontWeight")), 0);

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
