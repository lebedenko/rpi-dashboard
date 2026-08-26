#include <QFile>
#include <QProcess>
#include <QtTest>

class DashboardStartupTest : public QObject {
  Q_OBJECT

 private slots:
  void sidebarUsesIconOnlySafeLayout();
  void sidebarDrawsInsetClosedBorder();
  void sidebarButtonExposesInteractionStates();
  void sidebarIconsAreTintableSvgResources();
  void clockSidebarIsPackagedAndUsesThemeRoles();
  void iconFallbackColorsMatchThemeRoles();
  void declaresAndUsesTypographyRoles();
  void deviceCardSourcesAndChevronArePackaged();
  void initializesQmlAndKeepsRunning();
};

void DashboardStartupTest::
    clockSidebarIsPackagedAndUsesThemeRoles() {  // NOLINT(readability-convert-member-functions-to-static)
  QFile clockQml(QStringLiteral(DASHBOARD_CLOCK_SIDEBAR_QML));
  QVERIFY2(clockQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(clockQml.errorString()));
  const auto source = QString::fromUtf8(clockQml.readAll());

  QVERIFY(source.contains(QStringLiteral("Qt.formatTime(root.currentTimestamp, \"hh:mm\")")));
  QVERIFY(source.contains(QStringLiteral("Qt.formatDate(root.currentTimestamp, \"ddd dd MMM\")")));
  QVERIFY(source.contains(QStringLiteral("interval: 1000")));
  QVERIFY(source.contains(QStringLiteral("Accessible.role: Accessible.StaticText")));
  QVERIFY(source.contains(QStringLiteral("color: Theme.primaryAccent")));
  QVERIFY(source.contains(QStringLiteral("color: Theme.violetAccent")));
  QVERIFY(source.contains(QStringLiteral("font.pixelSize: Theme.clockTimeTextSize")));
  QVERIFY(source.contains(QStringLiteral("font.pixelSize: Theme.clockDateTextSize")));
  QCOMPARE(source.count(QStringLiteral("font.family: Theme.sansFontFamily")), 3);
  QVERIFY(source.contains(QStringLiteral("startX: Theme.sidebarCornerRadius")));
  QVERIFY(source.contains(QStringLiteral("startY: 0")));
  QVERIFY(source.contains(QStringLiteral("x: sidebarBackground.width - Theme.sidebarChamfer; y: 0")));
  QVERIFY(source.contains(QStringLiteral("x: 0; y: Theme.sidebarCornerRadius")));

  QFile themeQml(QStringLiteral(DASHBOARD_THEME_QML));
  QVERIFY2(themeQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(themeQml.errorString()));
  const auto themeSource = QString::fromUtf8(themeQml.readAll());
  QVERIFY(themeSource.contains(QStringLiteral("statusSidebarWidth: 144")));
  QVERIFY(themeSource.contains(QStringLiteral("clockTimeTextSize: 48")));
  QVERIFY(themeSource.contains(QStringLiteral("clockDateTextSize: 14")));
}

void DashboardStartupTest::sidebarUsesIconOnlySafeLayout() {  // NOLINT(readability-convert-member-functions-to-static)
  QFile themeQml(QStringLiteral(DASHBOARD_THEME_QML));
  QVERIFY2(themeQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(themeQml.errorString()));
  const auto themeSource = QString::fromUtf8(themeQml.readAll());
  QVERIFY(themeSource.contains(QStringLiteral("sidebarWidth: 64")));
  QVERIFY(themeSource.contains(QStringLiteral("touchTarget: 48")));
  QVERIFY(themeSource.contains(QStringLiteral("displaySafeInset: 10")));
  QVERIFY(themeSource.contains(QStringLiteral("sidebarCornerRadius: 4")));
  QVERIFY(themeSource.contains(QStringLiteral("navigationFrameCornerRadius: 2")));

  QFile mainQml(QStringLiteral(DASHBOARD_MAIN_QML));
  QVERIFY2(mainQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(mainQml.errorString()));
  const auto source = QString::fromUtf8(mainQml.readAll());
  QVERIFY(source.contains(QStringLiteral("Layout.preferredWidth: Theme.sidebarWidth")));
  QVERIFY(source.contains(QStringLiteral("Layout.leftMargin: Theme.displaySafeInset")));
  QVERIFY(source.contains(QStringLiteral("Layout.topMargin: Theme.displaySafeInset")));
  QVERIFY(source.contains(QStringLiteral("Layout.bottomMargin: Theme.displaySafeInset")));
  QVERIFY(source.contains(QStringLiteral("anchors.top: parent.top")));
  QVERIFY(source.contains(QStringLiteral("anchors.topMargin: Theme.spacingSmall")));
  QVERIFY(source.contains(QStringLiteral("anchors.horizontalCenter: parent.horizontalCenter")));
  QVERIFY(!source.contains(QStringLiteral("anchors.centerIn: parent")));
  QVERIFY(source.contains(QStringLiteral("spacing: Theme.spacingSmall")));
  QCOMPARE(source.count(QStringLiteral("SidebarButton {")), 4);
  QVERIFY(!source.contains(QStringLiteral("delegate: Loader")));
  QVERIFY(!source.contains(QStringLiteral("qsTr(\"Dashboard\")")));
  QVERIFY(!source.contains(QStringLiteral("Binding {")));
  QVERIFY(source.contains(QStringLiteral("tooltipText: qsTr(\"Overview\")")));
  QVERIFY(source.contains(QStringLiteral("onClicked: pageStack.currentIndex = 0")));
}

void DashboardStartupTest::sidebarDrawsInsetClosedBorder() {  // NOLINT(readability-convert-member-functions-to-static)
  QFile mainQml(QStringLiteral(DASHBOARD_MAIN_QML));
  QVERIFY2(mainQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(mainQml.errorString()));
  const auto source = QString::fromUtf8(mainQml.readAll());

  const auto sidebarStart = source.indexOf(QStringLiteral("id: sidebarSurface"));
  const auto contentStart = source.indexOf(QStringLiteral("Column {"), sidebarStart);
  QVERIFY(sidebarStart >= 0);
  QVERIFY(contentStart > sidebarStart);

  const auto sidebarSurface = source.sliced(sidebarStart, contentStart - sidebarStart);
  QVERIFY(source.contains(QStringLiteral("id: sidebarBackground")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("fillColor: Theme.surface")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("strokeColor: Theme.passiveBorder")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("strokeWidth: 1")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("startX: Theme.sidebarChamfer")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("PathArc {")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("radiusX: Theme.sidebarCornerRadius")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("radiusY: Theme.sidebarCornerRadius")));
  QVERIFY(sidebarSurface.contains(
      QStringLiteral("PathLine { x: sidebarBackground.width - Theme.sidebarChamfer; y: sidebarBackground.height }")));
  QVERIFY(
      sidebarSurface.contains(QStringLiteral("PathLine { x: 0; y: sidebarBackground.height - Theme.sidebarChamfer }")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("PathLine { x: Theme.sidebarChamfer; y: 0 }")));
  QVERIFY(!source.contains(QStringLiteral("id: sidebarSeparator")));
  QCOMPARE(sidebarSurface.count(QStringLiteral("ShapePath {")), 1);
  QCOMPARE(sidebarSurface.count(QStringLiteral("PathLine")), 7);
  QCOMPARE(sidebarSurface.count(QStringLiteral("PathArc")), 1);
}

void DashboardStartupTest::
    sidebarButtonExposesInteractionStates() {  // NOLINT(readability-convert-member-functions-to-static)
  QFile buttonQml(QStringLiteral(DASHBOARD_SIDEBAR_BUTTON_QML));
  QVERIFY2(buttonQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(buttonQml.errorString()));
  const auto source = QString::fromUtf8(buttonQml.readAll());

  QVERIFY(source.contains(QStringLiteral("display: AbstractButton.IconOnly")));
  QVERIFY(source.contains(QStringLiteral("property url iconSource")));
  QVERIFY(source.contains(QStringLiteral("source: root.iconSource")));
  QVERIFY(source.contains(QStringLiteral("activeFocusOnTab: true")));
  QVERIFY(source.contains(QStringLiteral("Accessible.name: root.tooltipText")));
  QVERIFY(source.contains(QStringLiteral("root.down ? 0.96 : 1")));
  QVERIFY(source.contains(QStringLiteral("root.hovered")));
  QVERIFY(source.contains(QStringLiteral("root.selected")));
  QVERIFY(source.contains(QStringLiteral("root.selected ? Theme.selectedSurface")));
  QVERIFY(source.contains(QStringLiteral("root.activeFocus")));
  QVERIFY(source.contains(QStringLiteral("acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad")));
  QVERIFY(source.contains(QStringLiteral("visible: pointerHover.hovered")));
  QVERIFY(!source.contains(QStringLiteral("ToolTip.visible: root.hovered || root.activeFocus")));
  QVERIFY(source.contains(QStringLiteral("strokeWidth: root.activeFocus ? 2 : root.selected ? 1 : 0")));
  QVERIFY(source.contains(QStringLiteral("startX: Theme.navigationFrameChamfer")));
  QVERIFY(source.contains(QStringLiteral("radiusX: Theme.navigationFrameCornerRadius")));
  QVERIFY(source.contains(QStringLiteral("radiusY: Theme.navigationFrameCornerRadius")));
  QVERIFY(source.contains(QStringLiteral("PathLine { x: root.width; y: root.height - Theme.navigationFrameChamfer }")));
  QVERIFY(source.contains(QStringLiteral("PathLine { x: root.width - Theme.navigationFrameChamfer; y: root.height }")));
  QVERIFY(source.contains(QStringLiteral("PathLine { x: 0; y: Theme.navigationFrameChamfer }")));
  QCOMPARE(source.count(QStringLiteral("PathArc")), 2);
  QCOMPARE(source.count(QStringLiteral("PathLine")), 6);
}

void DashboardStartupTest::
    sidebarIconsAreTintableSvgResources() {  // NOLINT(readability-convert-member-functions-to-static)
  const QStringList iconNames = {QStringLiteral("overview.svg"), QStringLiteral("systems.svg"),
                                 QStringLiteral("projects.svg"), QStringLiteral("weather.svg")};
  for (const auto& iconName : iconNames) {
    QFile icon(QStringLiteral(DASHBOARD_ICON_DIRECTORY) + QLatin1Char('/') + iconName);
    QVERIFY2(icon.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(icon.errorString()));
    const auto source = QString::fromUtf8(icon.readAll());
    QVERIFY2(source.contains(QStringLiteral("viewBox=\"0 0 24 24\"")), qPrintable(iconName));
    QVERIFY2(source.contains(QStringLiteral("stroke-width=\"2\"")), qPrintable(iconName));
    QVERIFY2(!source.contains(QStringLiteral("#000")), qPrintable(iconName));
  }

  QFile weatherIcon(QStringLiteral(DASHBOARD_ICON_DIRECTORY) + QStringLiteral("/weather.svg"));
  QVERIFY2(weatherIcon.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(weatherIcon.errorString()));
  const auto weatherSource = QString::fromUtf8(weatherIcon.readAll());
  QVERIFY(weatherSource.contains(QStringLiteral("M12 2v2")));
  QVERIFY(weatherSource.contains(QStringLiteral("M13 22H6")));

  QFile mainQml(QStringLiteral(DASHBOARD_MAIN_QML));
  QVERIFY2(mainQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(mainQml.errorString()));
  const auto mainSource = QString::fromUtf8(mainQml.readAll());
  QCOMPARE(mainSource.count(QStringLiteral("iconSource: Qt.resolvedUrl(\"icons/")), 4);
}

void DashboardStartupTest::
    iconFallbackColorsMatchThemeRoles() {  // NOLINT(readability-convert-member-functions-to-static)
  const auto verifyIconsContain = [](const QStringList& icon_names, const QString& expected_color) {
    for (const auto& icon_name : icon_names) {
      QFile icon(QStringLiteral(DASHBOARD_ICON_DIRECTORY) + QLatin1Char('/') + icon_name);
      QVERIFY2(icon.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(icon.errorString()));
      const auto source = QString::fromUtf8(icon.readAll());
      QVERIFY2(source.contains(expected_color), qPrintable(icon_name));
    }
  };

  verifyIconsContain({QStringLiteral("overview.svg"), QStringLiteral("systems.svg"), QStringLiteral("projects.svg"),
                      QStringLiteral("weather.svg")},
                     QStringLiteral("#F2F7FC"));
  verifyIconsContain(
      {QStringLiteral("detail-arch.svg"), QStringLiteral("detail-cores.svg"), QStringLiteral("detail-cpu.svg"),
       QStringLiteral("detail-hardware.svg"), QStringLiteral("detail-kernel.svg"), QStringLiteral("detail-memory.svg"),
       QStringLiteral("detail-os.svg")},
      QStringLiteral("#20D4F7"));
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

void DashboardStartupTest::
    deviceCardSourcesAndChevronArePackaged() {  // NOLINT(readability-convert-member-functions-to-static)
  for (const auto& path : {QStringLiteral(DASHBOARD_DEVICE_CARD_QML), QStringLiteral(DASHBOARD_OVERVIEW_PAGE_QML)}) {
    QFile source(path);
    QVERIFY2(source.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(source.errorString()));
    const auto contents = QString::fromUtf8(source.readAll());
    QVERIFY(!contents.isEmpty());
  }

  QFile deviceCard(QStringLiteral(DASHBOARD_DEVICE_CARD_QML));
  QVERIFY2(deviceCard.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(deviceCard.errorString()));
  const auto cardSource = QString::fromUtf8(deviceCard.readAll());
  QVERIFY(cardSource.contains(QStringLiteral("Loader {")));
  QVERIFY(cardSource.contains(QStringLiteral("active: root.expanded")));
  QVERIFY(!cardSource.contains(QStringLiteral("FooterButton")));
  QVERIFY(!cardSource.contains(QStringLiteral("deviceFooter")));
  QVERIFY(!cardSource.contains(QStringLiteral("selectionRequested")));
  QVERIFY(cardSource.contains(QStringLiteral("root.online ? Theme.onlineFrame : Theme.passiveBorder")));
  QVERIFY(!cardSource.contains(QStringLiteral("Timer {")));
  QVERIFY(!cardSource.contains(QStringLiteral("Behavior on height")));

  QFile chevron(QStringLiteral(DASHBOARD_ICON_DIRECTORY) + QStringLiteral("/chevron.svg"));
  QVERIFY2(chevron.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(chevron.errorString()));
  const auto chevronSource = QString::fromUtf8(chevron.readAll());
  QVERIFY(chevronSource.contains(QStringLiteral("viewBox=\"0 0 24 24\"")));
  QVERIFY(chevronSource.contains(QStringLiteral("stroke-width=\"2\"")));
  QVERIFY(!chevronSource.contains(QStringLiteral("#000")));
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

  const auto startupErrors = QString::fromUtf8(dashboard.readAllStandardError());
  QVERIFY2(!startupErrors.contains(QStringLiteral("Unsupported image format")), qPrintable(startupErrors));
  QVERIFY2(!startupErrors.contains(QStringLiteral("Error decoding")), qPrintable(startupErrors));

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
