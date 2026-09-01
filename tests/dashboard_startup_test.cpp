#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
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
  void bundledFontsArePackaged();
  void deviceCardSourcesAndChevronArePackaged();
  void migratedSurfacesUseReusableFrame();
  void initializesQmlAndKeepsRunning();
  void reportsWeatherCredentialFailuresWithoutSecrets();
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
  QVERIFY(source.count(QStringLiteral("font.family: Theme.sansFontFamily")) >= 3);
  QVERIFY(source.contains(QStringLiteral("topLeft: { rounded: Theme.radiusMedium }")));
  QVERIFY(source.contains(QStringLiteral("topRight: { chamfered: Theme.chamferLarge }")));
  QVERIFY(source.contains(QStringLiteral("bottomLeft: { chamfered: Theme.chamferLarge }")));

  QFile themeQml(QStringLiteral(DASHBOARD_THEME_QML));
  QVERIFY2(themeQml.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(themeQml.errorString()));
  const auto themeSource = QString::fromUtf8(themeQml.readAll());
  QVERIFY(themeSource.contains(QStringLiteral("statusSidebarWidth: 144")));
  QVERIFY(themeSource.contains(QStringLiteral("clockTimeTextSize: 48")));
  QVERIFY(themeSource.contains(QStringLiteral("clockDateTextSize: 14")));
}

void DashboardStartupTest::sidebarUsesIconOnlySafeLayout() {  // NOLINT(readability-convert-member-functions-to-static)
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
  QVERIFY(sidebarSurface.contains(QStringLiteral("Frame {")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("backgroundColor: Theme.surface")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("color: Theme.passiveBorder")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("topLeft: { chamfered: Theme.chamferLarge }")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("topRight: { rounded: Theme.radiusMedium }")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("bottomRight: { chamfered: Theme.chamferLarge }")));
  QVERIFY(sidebarSurface.contains(QStringLiteral("bottomLeft: { chamfered: Theme.chamferLarge }")));
  QVERIFY(!source.contains(QStringLiteral("id: sidebarSeparator")));
  QCOMPARE(sidebarSurface.count(QStringLiteral("ShapePath {")), 0);
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
  QVERIFY(source.contains(QStringLiteral("lineWidth: root.activeFocus ? 2 : root.selected ? 1 : 0")));
  QVERIFY(source.contains(QStringLiteral("topLeft: { chamfered: Theme.chamferMedium }")));
  QVERIFY(source.contains(QStringLiteral("topRight: { rounded: Theme.radiusSmall }")));
  QVERIFY(source.contains(QStringLiteral("bottomRight: { chamfered: Theme.chamferMedium }")));
  QVERIFY(source.contains(QStringLiteral("bottomLeft: { rounded: Theme.radiusSmall }")));
  QVERIFY(!source.contains(QStringLiteral("ShapePath {")));
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

  QVERIFY(themeSource.contains(QStringLiteral("bundledFontsReady")));
  QCOMPARE(themeSource.count(QStringLiteral("property FontLoader")), 7);
  QVERIFY(themeSource.contains(QStringLiteral("bundledSansFontFamily")));
  QVERIFY(themeSource.contains(QStringLiteral("bundledFixedFontFamily")));
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

void DashboardStartupTest::bundledFontsArePackaged() {  // NOLINT(readability-convert-member-functions-to-static)
  const QStringList fonts{QStringLiteral("Rajdhani-Light.ttf"),      QStringLiteral("Rajdhani-Regular.ttf"),
                          QStringLiteral("Rajdhani-Medium.ttf"),     QStringLiteral("Rajdhani-SemiBold.ttf"),
                          QStringLiteral("JetBrainsMono-Light.ttf"), QStringLiteral("JetBrainsMono-Regular.ttf"),
                          QStringLiteral("JetBrainsMono-Medium.ttf")};
  for (const auto& name : fonts) {
    QFile font(QStringLiteral(DASHBOARD_FONT_DIRECTORY) + QLatin1Char('/') + name);
    QVERIFY2(font.open(QIODevice::ReadOnly), qPrintable(name));
    QVERIFY2(font.size() > 100'000, qPrintable(name));
  }
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
  QVERIFY(cardSource.contains(QStringLiteral("root.statusColor")));
  QVERIFY(cardSource.contains(QStringLiteral("qsTr(\"REGISTERED\")")));
  QVERIFY(cardSource.contains(QStringLiteral("qsTr(\"STALE\")")));
  QVERIFY(!cardSource.contains(QStringLiteral("Timer {")));
  QVERIFY(!cardSource.contains(QStringLiteral("Behavior on height")));

  QFile chevron(QStringLiteral(DASHBOARD_ICON_DIRECTORY) + QStringLiteral("/chevron.svg"));
  QVERIFY2(chevron.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(chevron.errorString()));
  const auto chevronSource = QString::fromUtf8(chevron.readAll());
  QVERIFY(chevronSource.contains(QStringLiteral("viewBox=\"0 0 24 24\"")));
  QVERIFY(chevronSource.contains(QStringLiteral("stroke-width=\"2\"")));
  QVERIFY(!chevronSource.contains(QStringLiteral("#000")));
}

void DashboardStartupTest::
    migratedSurfacesUseReusableFrame() {  // NOLINT(readability-convert-member-functions-to-static)
  const QStringList migratedSources = {
      QStringLiteral(DASHBOARD_MAIN_QML), QStringLiteral(DASHBOARD_CLOCK_SIDEBAR_QML),
      QStringLiteral(DASHBOARD_SIDEBAR_BUTTON_QML), QStringLiteral(DASHBOARD_DEVICE_CARD_QML),
      QStringLiteral(DASHBOARD_SYSTEM_PAGE_QML), QStringLiteral(DASHBOARD_SYSTEM_METRIC_PANEL_QML)};
  for (const auto& path : migratedSources) {
    QFile source(path);
    QVERIFY2(source.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(source.errorString()));
    const auto contents = QString::fromUtf8(source.readAll());
    QVERIFY2(contents.contains(QStringLiteral("Frame {")), qPrintable(path));
    QVERIFY2(!contents.contains(QStringLiteral("ShapePath {")), qPrintable(path));
    QVERIFY2(!contents.contains(QStringLiteral("import QtQuick.Shapes")), qPrintable(path));
  }

  QFile frame(QStringLiteral(DASHBOARD_FRAME_QML));
  QVERIFY(frame.open(QIODevice::ReadOnly | QIODevice::Text));
  const auto frameSource = QString::fromUtf8(frame.readAll());
  QCOMPARE(frameSource.count(QStringLiteral("ShapePath {")), 1);

  QFile device(QStringLiteral(DASHBOARD_DEVICE_CARD_QML));
  QVERIFY(device.open(QIODevice::ReadOnly | QIODevice::Text));
  const auto deviceSource = QString::fromUtf8(device.readAll());
  QVERIFY(deviceSource.contains(
      QStringLiteral("corners: ({ topRight: { chamfered: Theme.chamferLarge }, bottomRight: { chamfered: "
                     "Theme.chamferLarge } })")));
  QVERIFY(deviceSource.contains(QStringLiteral("lineWidth: chevron.activeFocus ? 2 : 1")));
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

void DashboardStartupTest::
    reportsWeatherCredentialFailuresWithoutSecrets() {  // NOLINT(readability-convert-member-functions-to-static)
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  QFile config(directory.filePath(QStringLiteral("config.toml")));
  QVERIFY(config.open(QIODevice::WriteOnly | QIODevice::Text));
  config.write(R"([credentials]
openweather_api_key_file = "/missing/openweather-do-not-log"
ipgeolocation_api_key_file = "/missing/ipgeolocation-do-not-log"

[weather]
provider = "openweather"
refresh_interval_seconds = 600

[weather.location]
automatic_provider = "ipgeolocation"
)");
  config.close();

  QProcess dashboard;
  auto environment = QProcessEnvironment::systemEnvironment();
  environment.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
  environment.insert(QStringLiteral("QT_FORCE_STDERR_LOGGING"), QStringLiteral("1"));
  environment.remove(QStringLiteral("OPENWEATHER_API_KEY_FILE"));
  environment.remove(QStringLiteral("IPGEOLOCATION_API_KEY_FILE"));
  dashboard.setProcessEnvironment(environment);
  dashboard.setReadChannel(QProcess::StandardError);
  dashboard.setProgram(QStringLiteral(DASHBOARD_EXECUTABLE));
  dashboard.setArguments({QStringLiteral("--config"), config.fileName()});
  dashboard.start();

  QVERIFY2(dashboard.waitForStarted(), qPrintable(dashboard.errorString()));
  QVERIFY2(dashboard.waitForReadyRead(2000), qPrintable(dashboard.errorString()));
  QByteArray standardError = dashboard.readAllStandardError();
  if (!standardError.contains("IP geolocation credential file could not be read")) {
    dashboard.waitForReadyRead(500);
    standardError += dashboard.readAllStandardError();
  }
  dashboard.terminate();
  if (!dashboard.waitForFinished(3000)) {
    dashboard.kill();
    dashboard.waitForFinished(3000);
  }

  const auto diagnostics = QString::fromUtf8(standardError + dashboard.readAllStandardError());
  QVERIFY2(diagnostics.contains(QStringLiteral("OpenWeather credential file could not be read")),
           qPrintable(diagnostics));
  QVERIFY2(diagnostics.contains(QStringLiteral("IP geolocation credential file could not be read")),
           qPrintable(diagnostics));
  QVERIFY(!diagnostics.contains(QStringLiteral("do-not-log")));
}

QTEST_GUILESS_MAIN(DashboardStartupTest)

#include "dashboard_startup_test.moc"
