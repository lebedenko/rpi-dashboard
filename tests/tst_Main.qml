import Rpi.Dashboard
import QtQuick
import QtTest

Item {
    id: root

    function deviceEntry(number, name, cpuMetric = "—", memoryMetric = "—", temperatureMetric = "—", uptimeMetric = "—", cpuUsageRatio = -1, memoryUsageRatio = -1) {
        return {
            "deviceNumber": number,
            "hostname": name,
            "online": true,
            "cpuMetric": cpuMetric,
            "memoryMetric": memoryMetric,
            "cpuUsageRatio": cpuUsageRatio,
            "memoryUsageRatio": memoryUsageRatio,
            "temperatureMetric": temperatureMetric,
            "uptimeMetric": uptimeMetric,
            "osDescription": "—",
            "kernelDescription": "—",
            "architecture": "—",
            "hardwareDescription": "—",
            "cpuDescription": "—",
            "coreDescription": "—",
            "totalMemory": "—"
        };
    }

    width: 1480
    height: 320

    TestCase {
        property var dashboardWindow: null
        property var dashboardComponent: null
        property var fakeService: null
        property var fakeMetricsService: null
        property var fakeProjectsService: null

        function init() {
            fakeService = fakeServiceComponent.createObject(root);
            fakeMetricsService = fakeMetricsServiceComponent.createObject(root);
            fakeProjectsService = fakeProjectsServiceComponent.createObject(root);
            dashboardComponent = Qt.createComponent("qrc:/qt/qml/Rpi/Dashboard/qml/Main.qml");
            verify(dashboardComponent.status === Component.Ready, dashboardComponent.errorString());
            dashboardWindow = dashboardComponent.createObject(null, {
                "windowed": true,
                "windowWidth": 1480,
                "windowHeight": 320,
                "sysInfoService": fakeService,
                "sysMetricsService": fakeMetricsService,
                "projectsService": fakeProjectsService
            });
            verify(dashboardWindow !== null);
            dashboardWindow.show();
            tryCompare(dashboardWindow, "visible", true);
            dashboardWindow.requestActivate();
            tryCompare(dashboardWindow, "active", true);
        }

        function cleanup() {
            dashboardWindow.close();
            dashboardWindow.destroy();
            dashboardWindow = null;
            dashboardComponent.destroy();
            dashboardComponent = null;
            fakeService.destroy();
            fakeService = null;
            fakeMetricsService.destroy();
            fakeMetricsService = null;
            fakeProjectsService.destroy();
            fakeProjectsService = null;
        }

        function test_exactDevelopmentGeometry() {
            compare(dashboardWindow.width, 1480);
            compare(dashboardWindow.height, 320);
            compare(dashboardWindow.visibility, Window.Windowed);
        }

        function test_vibrantPaletteUsesSpecifiedSemanticColors() {
            compare(Theme.background, "#020a13");
            compare(Theme.surface, "#061321");
            compare(Theme.surfaceElevated, "#0a1a2b");
            compare(Theme.surfaceRaised, "#10283c");
            compare(Theme.cardSurface, "#041321");
            compare(Theme.selectedSurface, "#09283c");
            compare(Theme.textPrimary, "#f2f7fc");
            compare(Theme.textSecondary, "#b7c7d9");
            compare(Theme.textMuted, "#8295ac");
            compare(Theme.chartText, Theme.textMuted);
            compare(Theme.primaryAccent, "#19d3f3");
            compare(Theme.focusAccent, "#5de7ff");
            compare(Theme.cardAccent, "#20d4f7");
            compare(Theme.cpuSeries, "#36b9ff");
            compare(Theme.memorySeries, "#a66cff");
            compare(Theme.onlineStatus, "#50f0a0");
            compare(dashboardWindow.color, Theme.background);
        }

        function test_pageSelectionAndBoundedNavigation() {
            compare(dashboardWindow.currentPageIndex, 0);
            keyClick(Qt.Key_Left);
            tryCompare(dashboardWindow, "currentPageIndex", 0);
            keyClick(Qt.Key_Right);
            tryCompare(dashboardWindow, "currentPageIndex", 1);
            keyClick(Qt.Key_Right);
            keyClick(Qt.Key_Right);
            keyClick(Qt.Key_Right);
            tryCompare(dashboardWindow, "currentPageIndex", 3);
            keyClick(Qt.Key_Home);
            tryCompare(dashboardWindow, "currentPageIndex", 0);
        }

        function test_ctrlQRequestsNormalExit() {
            keyClick(Qt.Key_Q, Qt.ControlModifier);
            tryCompare(dashboardWindow, "visible", false);
        }

        function test_touchTargetsSelectPages() {
            const buttonNames = ["overviewButton", "systemsButton", "projectsButton", "weatherButton"];
            for (let index = 0; index < buttonNames.length; ++index) {
                const button = findChild(dashboardWindow.contentItem, buttonNames[index]);
                verify(!!button, "Object exists");
                compare(button.width, 48);
                compare(button.height, 48);
                mouseClick(button);
                tryCompare(dashboardWindow, "currentPageIndex", index);
            }
        }

        function test_insetSidebarGeometry() {
            const sidebar = findChild(dashboardWindow.contentItem, "sidebarSurface");
            const pageStack = findChild(dashboardWindow.contentItem, "pageStack");
            verify(!!sidebar);
            verify(!!pageStack);
            compare(sidebar.x, 10);
            compare(sidebar.y, 10);
            compare(sidebar.width, 64);
            compare(sidebar.height, 300);
            compare(pageStack.x, 74);
            compare(pageStack.width, 1252);

            const buttonNames = ["overviewButton", "systemsButton", "projectsButton", "weatherButton"];
            let previousBottom = -1;
            for (const buttonName of buttonNames) {
                const button = findChild(sidebar, buttonName);
                verify(!!button);
                compare(button.width, 48);
                compare(button.height, 48);
                const position = button.mapToItem(dashboardWindow.contentItem, 0, 0);
                compare(position.x, 18);
                verify(position.y >= Theme.displaySafeInset);
                verify(position.y + button.height <= dashboardWindow.height - Theme.displaySafeInset);
                if (previousBottom >= 0)
                    compare(position.y - previousBottom, 8);
                previousBottom = position.y + button.height;
            }

            const firstButton = findChild(sidebar, buttonNames[0]);
            const lastButton = findChild(sidebar, buttonNames[buttonNames.length - 1]);
            compare(firstButton.mapToItem(dashboardWindow.contentItem, 0, 0).y, 18);
            compare(lastButton.mapToItem(dashboardWindow.contentItem, 0, 0).y + lastButton.height, 234);
        }

        function test_clockSidebarGeometryFormattingAndTheme() {
            const sidebar = findChild(dashboardWindow.contentItem, "clockSidebar");
            const timeLabel = findChild(sidebar, "clockTimeLabel");
            const dateLabel = findChild(sidebar, "clockDateLabel");
            verify(!!sidebar);
            verify(!!timeLabel);
            verify(!!dateLabel);

            compare(sidebar.x, 1326);
            compare(sidebar.y, 10);
            compare(sidebar.width, 144);
            compare(sidebar.height, 300);
            verify(timeLabel.mapToItem(dashboardWindow.contentItem, 0, 0).y
                   < dateLabel.mapToItem(dashboardWindow.contentItem, 0, 0).y);
            verify(timeLabel.mapToItem(dashboardWindow.contentItem, 0, 0).y >= Theme.displaySafeInset);
            verify(dateLabel.mapToItem(dashboardWindow.contentItem, 0, dateLabel.height).y
                   <= dashboardWindow.height - Theme.displaySafeInset);

            compare(sidebar.timeText, Qt.formatTime(sidebar.currentTimestamp, "hh:mm"));
            compare(sidebar.dateText, Qt.formatDate(sidebar.currentTimestamp, "ddd dd MMM"));
            compare(timeLabel.text, sidebar.timeText);
            compare(dateLabel.text, sidebar.dateText);
            compare(timeLabel.color, Theme.primaryAccent);
            compare(dateLabel.color, Theme.violetAccent);
            compare(timeLabel.font.pixelSize, Theme.clockTimeTextSize);
            compare(dateLabel.font.pixelSize, Theme.clockDateTextSize);
            compare(timeLabel.font.family, Theme.sansFontFamily);
            compare(dateLabel.font.family, Theme.sansFontFamily);
            compare(sidebar.Accessible.role, Accessible.StaticText);
            compare(sidebar.Accessible.name, sidebar.accessibleText);
            verify(sidebar.Accessible.name.includes(sidebar.timeText));
            verify(sidebar.Accessible.name.includes(sidebar.dateText));
            compare(sidebar.activeFocus, false);
        }

        function test_projectsPageGeometrySelectionAndStatusRail() {
            const projectsButton = findChild(dashboardWindow.contentItem, "projectsButton");
            mouseClick(projectsButton);
            tryCompare(dashboardWindow, "currentPageIndex", 2);
            const list = findChild(dashboardWindow.contentItem, "projectList");
            const first = findChild(dashboardWindow.contentItem, "projectRow0");
            verify(!!list);
            verify(!!first);
            tryVerify(() => !!findChild(dashboardWindow.contentItem, "projectRow1"));
            const second = findChild(dashboardWindow.contentItem, "projectRow1");
            compare(list.width, 330);
            compare(first.height, 56);
            compare(second.y - first.y - first.height, 6);
            compare(first.Accessible.role, Accessible.ListItem);
            verify(first.Accessible.name.includes("FAILED"));
            mouseClick(second);
            tryCompare(fakeProjectsService, "selectedProjectIndex", 1);
            keyClick(Qt.Key_F5);
            tryCompare(second, "activeFocus", true);
            const ci = findChild(dashboardWindow.contentItem, "globalCiHealth");
            const runners = findChild(dashboardWindow.contentItem, "globalRunnerHealth");
            compare(ci.text, "FAILED CI");
            compare(runners.text, "— RUNNERS");
            compare(ci.activeFocus, false);
        }

        function test_f5FocusesCurrentPage() {
            keyClick(Qt.Key_Right);
            tryCompare(dashboardWindow, "currentPageIndex", 1);
            keyClick(Qt.Key_F5);
            tryCompare(dashboardWindow, "currentPageHasFocus", true);
        }

        function test_localDeviceContentAndExpandedState() {
            const card = findChild(dashboardWindow.contentItem, "deviceCard0");
            verify(!!card);
            compare(card.deviceNumber, "01");
            compare(card.hostname, "PI-DASH");
            compare(card.online, true);
            compare(card.cpuMetric, "42%");
            compare(card.memoryMetric, "68%");
            compare(card.cpuUsageRatio, 0.42);
            compare(card.memoryUsageRatio, 0.68);
            compare(card.temperatureMetric, "53°C");
            compare(card.uptimeMetric, "1d 2h");
            compare(card.osDescription, "Raspberry Pi OS · 13");
            compare(card.kernelDescription, "linux · 6.12-test");
            compare(card.architecture, "aarch64");
            compare(card.hardwareDescription, "Raspberry Pi · Raspberry Pi 5 Model B");
            compare(card.cpuDescription, "Broadcom · BCM2712");
            compare(card.coreDescription, "4 physical · 4 logical");
            compare(card.totalMemory, "8.0 GiB");
            compare(card.expanded, true);
            const overview = findChild(dashboardWindow.contentItem, "overviewPage");
            compare(card.height, overview.deviceList.height);
            tryCompare(card, "expandedContentLoaded", true);
            compare(card.chevronButton.width, 48);
            compare(card.chevronButton.height, 48);
            compare(card.chevronAccessibleName, "Collapse PI-DASH");
            compare(findChild(card, "deviceFooter"), null);
            compare(findChild(card, "selectActiveButton"), null);
            compare(findChild(card, "viewStreamsButton"), null);
            compare(findChild(card, "terminalButton"), null);
            compare(findChild(card, "moreButton"), null);
            verify(!!findChild(card, "resourceHistory"));
            verify(!!findChild(card, "historyGrid"));
            const series = findChild(card, "resourceHistorySeries");
            verify(!!series);
            compare(series.model, fakeMetricsService.usageHistoryModel);
            compare(series.cpuColor, Theme.cpuSeries);
            compare(series.memoryColor, Theme.memorySeries);
            compare(series.plotBackgroundColor, Theme.cardSurface);
            compare(series.transitionDuration, 350);
            compare(findChild(card, "cpuHistoryLegend").text, "CPU %");
            compare(findChild(card, "memoryHistoryLegend").text, "MEM %");
            const cpuLegendLine = findChild(card, "cpuHistoryLegendLine");
            const memoryLegendLine = findChild(card, "memoryHistoryLegendLine");
            compare(cpuLegendLine.width, 14);
            compare(cpuLegendLine.height, 2);
            compare(cpuLegendLine.color, Theme.cpuSeries);
            compare(memoryLegendLine.width, 14);
            compare(memoryLegendLine.height, 2);
            compare(memoryLegendLine.color, Theme.memorySeries);
            for (let index = 0; index < 4; ++index) {
                const guide = findChild(card, "historyHorizontalGuide" + index);
                verify(!!guide);
                compare(guide.height, 1);
                compare(guide.width, findChild(card, "historyGrid").width);
                compare(guide.color, Theme.chartGrid);
            }
            const baseline = findChild(card, "historyBaseline");
            verify(!!baseline);
            compare(baseline.color, Theme.chartAxis);
            verify(findChild(card, "historyLeftAxis") === null);
            verify(findChild(card, "historyVerticalGuide0") === null);
            const history = findChild(card, "resourceHistory");
            compare(history.width >= 600, true);
            compare(history.x + history.width <= card.width, true);
        }

        function test_chevronCollapsesAndExpandsWithPointerAndKeyboard() {
            const card = findChild(dashboardWindow.contentItem, "deviceCard0");
            const chevron = card.chevronButton;
            mouseClick(chevron);
            tryCompare(card, "expanded", false);
            compare(card.height, 64);
            tryCompare(card, "expandedContentLoaded", false);
            tryVerify(() => findChild(card, "resourceHistorySeries") === null);
            compare(card.chevronAccessibleName, "Expand PI-DASH");
            chevron.forceActiveFocus();
            keyClick(Qt.Key_Space);
            tryCompare(card, "expanded", true);
            tryCompare(card, "expandedContentLoaded", true);
        }

        function test_f5FocusesOverviewChevron() {
            keyClick(Qt.Key_F5);
            tryCompare(dashboardWindow, "currentPageHasFocus", true);
            const card = findChild(dashboardWindow.contentItem, "deviceCard0");
            compare(card.chevronButton.activeFocus, true);
        }

        function test_f5FocusesChevronAfterOnlyCardIsCollapsed() {
            const card = findChild(dashboardWindow.contentItem, "deviceCard0");
            mouseClick(card.chevronButton);
            tryCompare(card, "expanded", false);
            keyClick(Qt.Key_F5);
            tryCompare(card.chevronButton, "activeFocus", true);
        }

        function test_accordionSelectionFollowsExpansionAndSurvivesCollapse() {
            const overview = findChild(dashboardWindow.contentItem, "overviewPage");
            overview.deviceModel = listModelComponent.createObject(root);
            overview.deviceModel.append(deviceEntry("01", "FIRST"));
            overview.deviceModel.append(deviceEntry("02", "SECOND"));
            tryCompare(overview.deviceList, "count", 2);
            tryVerify(() => !!findChild(overview, "deviceCard1"));
            const first = findChild(overview, "deviceCard0");
            const second = findChild(overview, "deviceCard1");
            compare(overview.selectedIndex, 0);
            compare(first.selected, true);
            compare(second.selected, false);

            mouseClick(first.chevronButton);
            tryCompare(overview, "expandedIndex", -1);
            compare(overview.selectedIndex, 0);
            compare(first.selected, true);
            compare(first.expanded, false);
            compare(second.expanded, false);

            overview.expandCard(1);
            tryCompare(overview, "expandedIndex", 1);
            compare(overview.selectedIndex, 1);
            compare(first.selected, false);
            compare(second.selected, true);

            const expandedSecond = findChild(overview, "deviceCard1");
            verify(!!expandedSecond, "Object exists");
            overview.expandedIndex = -1;
            tryCompare(overview, "expandedIndex", -1);
            compare(overview.selectedIndex, 1);
            compare(expandedSecond.selected, true);
            compare(first.expanded, false);
            compare(expandedSecond.expanded, false);
        }

        function test_selectionNormalizesWhenModelCountChanges() {
            const overview = findChild(dashboardWindow.contentItem, "overviewPage");
            overview.deviceModel = listModelComponent.createObject(root);
            compare(overview.selectedIndex, -1);
            overview.deviceModel.append(deviceEntry("01", "FIRST"));
            tryCompare(overview, "selectedIndex", 0);
            overview.deviceModel.append(deviceEntry("02", "SECOND"));
            overview.expandCard(1);
            tryCompare(overview, "selectedIndex", 1);
            overview.deviceModel.remove(1);
            tryCompare(overview, "selectedIndex", 0);
            overview.deviceModel.clear();
            tryCompare(overview, "selectedIndex", -1);
        }

        function test_metricRolesAreForwarded() {
            const overview = findChild(dashboardWindow.contentItem, "overviewPage");
            overview.deviceModel = listModelComponent.createObject(root);
            overview.deviceModel.append(deviceEntry("01", "FORWARDED", "17%", "33%", "42°C", "2h", 0.17, 0.33));
            tryCompare(overview.deviceList, "count", 1);
            const card = findChild(overview, "deviceCard0");
            tryCompare(card, "hostname", "FORWARDED");
            compare(card.selected, true);
            compare(card.cpuMetric, "17%");
            compare(card.memoryMetric, "33%");
            compare(card.cpuUsageRatio, 0.17);
            compare(card.memoryUsageRatio, 0.33);
            compare(card.temperatureMetric, "42°C");
            compare(card.uptimeMetric, "2h");
        }

        function test_metricRailsReflectRatiosAndSeriesColors() {
            const card = findChild(dashboardWindow.contentItem, "deviceCard0");
            compare(card.cpuProgressFill.visible, true);
            compare(card.memoryProgressFill.visible, true);
            compare(Math.abs(card.cpuProgressFill.width / card.cpuProgressFill.parent.width - 0.42) < 0.001, true);
            compare(Math.abs(card.memoryProgressFill.width / card.memoryProgressFill.parent.width - 0.68) < 0.001, true);
            compare(card.cpuProgressFill.color, Theme.cpuSeries);
            compare(card.memoryProgressFill.color, Theme.memorySeries);
        }

        function test_unavailableMetricRatiosHideColoredFills() {
            const overview = findChild(dashboardWindow.contentItem, "overviewPage");
            overview.deviceModel = listModelComponent.createObject(root);
            overview.deviceModel.append(deviceEntry("01", "UNKNOWN"));
            tryCompare(overview.deviceList, "count", 1);
            const card = findChild(overview, "deviceCard0");
            tryCompare(card, "cpuMetric", "—");
            compare(card.cpuUsageRatio, -1);
            compare(card.memoryUsageRatio, -1);
            compare(card.cpuProgressFill.visible, false);
            compare(card.memoryProgressFill.visible, false);
        }

        function test_twoCardModelLeavesNextCardPeek() {
            const overview = findChild(dashboardWindow.contentItem, "overviewPage");
            overview.deviceModel = listModelComponent.createObject(root);
            overview.deviceModel.append(deviceEntry("01", "FIRST"));
            overview.deviceModel.append(deviceEntry("02", "SECOND"));
            tryCompare(overview.deviceList, "count", 2);
            tryVerify(() => {
                return !!findChild(overview, "deviceCard1");
            });
            const first = findChild(overview, "deviceCard0");
            const second = findChild(overview, "deviceCard1");
            verify(!!first);
            verify(!!second);
            compare(first.height, overview.deviceList.height - 8 - 24);
            compare(second.height, 64);
            compare(overview.deviceList.height - second.y, 24);
        }

        function test_firstMiddleAndFinalCardsPositionExpandedAtViewportStart() {
            const overview = findChild(dashboardWindow.contentItem, "overviewPage");
            overview.deviceModel = listModelComponent.createObject(root);
            overview.deviceModel.append(deviceEntry("01", "FIRST"));
            overview.deviceModel.append(deviceEntry("02", "MIDDLE"));
            overview.deviceModel.append(deviceEntry("03", "FINAL"));
            tryCompare(overview.deviceList, "count", 3);
            overview.expandCard(1);
            tryCompare(overview, "expandedIndex", 1);
            tryCompare(overview.deviceList, "contentY", 64 + 8);
            compare(findChild(overview, "deviceCard1").height, overview.deviceList.height - 8 - 24);
            overview.expandCard(2);
            tryCompare(overview, "expandedIndex", 2);
            tryCompare(overview.deviceList, "contentY", 2 * (64 + 8));
            compare(findChild(overview, "deviceCard2").height, overview.deviceList.height);
        }

        function test_serviceRefreshPreservesExpansionAndFocus() {
            const overview = findChild(dashboardWindow.contentItem, "overviewPage");
            const card = findChild(overview, "deviceCard0");
            keyClick(Qt.Key_F5);
            tryCompare(card.chevronButton, "activeFocus", true);
            const originalCard = card;
            fakeService.hostname = "pi-refreshed";
            tryCompare(card, "hostname", "PI-REFRESHED");
            compare(findChild(overview, "deviceCard0"), originalCard);
            compare(card.expanded, true);
            compare(card.chevronButton.activeFocus, true);
        }

        function test_metricRefreshPreservesExpansionAndFocus() {
            const card = findChild(dashboardWindow.contentItem, "deviceCard0");
            keyClick(Qt.Key_F5);
            tryCompare(card.chevronButton, "activeFocus", true);
            fakeMetricsService.cpuUsageRatio = 0.995;
            const series = findChild(card, "resourceHistorySeries");
            compare(series.model, fakeMetricsService.usageHistoryModel);
            fakeMetricsService.usageHistoryModel.append({"elapsedMilliseconds": 2000,
                                                         "cpuUsageRatio": 0.995,
                                                         "memoryUsageRatio": 0.68});
            fakeMetricsService.usageHistoryModel.append({"elapsedMilliseconds": 3000,
                                                         "memoryUsageRatio": 0.64});
            fakeMetricsService.usageHistoryModel.append({"elapsedMilliseconds": 4000,
                                                         "cpuUsageRatio": 0.73});
            fakeMetricsService.usageHistoryModel.append({"elapsedMilliseconds": 5000,
                                                         "cpuUsageRatio": 0.81,
                                                         "memoryUsageRatio": 0.66});
            compare(fakeMetricsService.usageHistoryModel.count, 5);
            compare(findChild(card, "resourceHistorySeries"), series);
            tryCompare(card, "cpuMetric", "100%");
            tryCompare(card, "cpuUsageRatio", 0.995);
            compare(card.expanded, true);
            compare(card.chevronButton.activeFocus, true);
            fakeMetricsService.uptimeSeconds = 30;
            tryCompare(card, "uptimeMetric", "<1m");
        }

        function test_deviceCardGeometryMatchesFixedViewportComposition() {
            const card = findChild(dashboardWindow.contentItem, "deviceCard0");
            const details = findChild(card, "deviceDetails");
            const history = findChild(card, "resourceHistory");
            verify(!!details);
            verify(!!history);
            const header = findChild(card, "deviceHeader");
            const chevron = card.chevronButton;
            for (const metricName of ["cpuMetricCell", "memoryMetricCell", "temperatureMetricCell", "uptimeMetricCell"]) {
                const metric = findChild(card, metricName);
                verify(!!metric);
                const position = metric.mapToItem(header, 0, 0);
                compare(position.y >= 0, true);
                compare(position.y + metric.height <= header.height, true);
            }
            const cpuMetric = findChild(card, "cpuMetricCell");
            const memoryMetric = findChild(card, "memoryMetricCell");
            const temperatureMetric = findChild(card, "temperatureMetricCell");
            const uptimeMetric = findChild(card, "uptimeMetricCell");
            const metricRegionWidth = cpuMetric.width + memoryMetric.width + temperatureMetric.width + uptimeMetric.width;
            compare(Math.abs(cpuMetric.width / metricRegionWidth - 0.28) < 0.001, true);
            compare(Math.abs(memoryMetric.width / metricRegionWidth - 0.28) < 0.001, true);
            compare(Math.abs(temperatureMetric.width / metricRegionWidth - 0.18) < 0.001, true);
            compare(Math.abs(uptimeMetric.width / metricRegionWidth - 0.26) < 0.001, true);
            compare(chevron.y >= 0, true);
            compare(chevron.y + chevron.height <= header.height, true);
            compare(Math.abs(details.width / (details.width + history.width) - 0.32) < 0.025, true);
            compare(findChild(card, "deviceFooter"), null);
            const detailsBottom = details.mapToItem(card, 0, details.height).y;
            const historyBottom = history.mapToItem(card, 0, history.height).y;
            compare(detailsBottom > card.height - 48, true);
            compare(historyBottom > card.height - 48, true);
            compare(detailsBottom <= card.height, true);
            compare(historyBottom <= card.height, true);
            const firstRow = findChild(card, "detailRow0");
            const lastRow = findChild(card, "detailRow6");
            verify(!!firstRow);
            verify(!!lastRow);
            compare(firstRow.valuePixelSize >= 14, true);
            compare(lastRow.y + lastRow.height <= details.height + 0.001, true);
            for (const labelName of ["historyLabel0", "historyLabel4", "historyValue0", "historyValue4"]) {
                const label = findChild(card, labelName);
                verify(!!label);
                const labelPosition = label.mapToItem(history, 0, 0);
                compare(labelPosition.x >= 0, true);
                compare(labelPosition.x + label.width <= history.width, true);
                compare(labelPosition.y >= 0, true);
                compare(labelPosition.y + label.height <= history.height, true);
            }
            const cardPosition = card.mapToItem(dashboardWindow.contentItem, 0, 0);
            compare(cardPosition.x >= 0, true);
            compare(cardPosition.y >= 0, true);
            compare(cardPosition.x + card.width <= dashboardWindow.width, true);
            compare(cardPosition.y + card.height <= dashboardWindow.height, true);
        }

        function test_layoutStaysWithinTargetBounds() {
            const pageStack = findChild(dashboardWindow.contentItem, "pageStack");
            verify(!!pageStack, "Object exists");
            compare(pageStack.x >= 0, true);
            compare(pageStack.y >= 0, true);
            compare(pageStack.x + pageStack.width <= dashboardWindow.width, true);
            compare(pageStack.y + pageStack.height <= dashboardWindow.height, true);
            const pageNames = ["overviewPage", "systemsPage", "projectsPage", "weatherPage"];
            for (const pageName of pageNames) {
                const page = findChild(dashboardWindow.contentItem, pageName);
                verify(!!page, "Object exists");
                compare(page.x >= 0, true);
                compare(page.y >= 0, true);
                compare(page.x + page.width <= pageStack.width, true);
                compare(page.y + page.height <= pageStack.height, true);
            }
        }

        name: "DashboardWindow"
        when: windowShown
    }

    Component {
        id: listModelComponent

        ListModel {
        }

    }

    Component {
        id: fakeServiceComponent

        QtObject {
            property string hostname: "pi-dash"
            property string osFamily: "linux"
            property string osId: "debian"
            property string osVersion: "13"
            property string osPrettyName: "Raspberry Pi OS"
            property string kernelType: "linux"
            property string kernelVersion: "6.12-test"
            property string architecture: "aarch64"
            property string hardwareManufacturer: "Raspberry Pi"
            property string hardwareModel: "Raspberry Pi 5 Model B"
            property string cpuVendor: "Broadcom"
            property string cpuModel: "BCM2712"
            property int physicalCoreCount: 4
            property int logicalCpuCount: 4
            property double totalMemoryBytes: 8.58993e+09
        }

    }

    Component {
        id: fakeProjectsServiceComponent

        QtObject {
            property ListModel projectModel: ListModel {
                ListElement { key: "owner/failed"; name: "failed"; branch: "main"; health: "failed"; status: "completed" }
                ListElement { key: "owner/healthy"; name: "healthy"; branch: "main"; health: "healthy"; status: "completed" }
            }
            property ListModel stageModel: ListModel {
                ListElement { key: "0"; name: "build"; branch: ""; health: "healthy"; status: "completed" }
            }
            property ListModel runHistoryModel: ListModel {
                ListElement { key: "42"; name: ""; branch: ""; health: "failed"; status: "" }
            }
            property int selectedProjectIndex: 0
            property string selectedRepository: selectedProjectIndex === 0 ? "failed" : "healthy"
            property string selectedBranch: "main"
            property string selectedRevision: "abc1234"
            property string selectedRun: "#42"
            property string selectedRunAge: "2m ago"
            property string duration: "3m"
            property string jobsSummary: "3/4"
            property string artifactSize: "2.0 MiB"
            property string deployStatus: "healthy"
            property int trackedCount: 2
            property int runningCount: 0
            property int failedCount: 1
            property string aggregateHealth: "failed"
            property int onlineRunnerCount: -1
            property int totalRunnerCount: -1
            property string state: "ready"
            property bool stale: false
            property string diagnostics: ""
            function selectProject(index) { selectedProjectIndex = index }
        }
    }

    Component {
        id: fakeMetricsServiceComponent

        QtObject {
            property double cpuUsageRatio: 0.42
            property double memoryUsageRatio: 0.68
            property double cpuTemperatureCelsius: 52.6
            property double uptimeSeconds: 93600
            property ListModel usageHistoryModel: ListModel {
                ListElement { elapsedMilliseconds: 0; cpuUsageRatio: 0.42; memoryUsageRatio: 0.68 }
            }
            signal currentMetricsChanged()
            onCpuUsageRatioChanged: currentMetricsChanged()
            onMemoryUsageRatioChanged: currentMetricsChanged()
            onCpuTemperatureCelsiusChanged: currentMetricsChanged()
            onUptimeSecondsChanged: currentMetricsChanged()
        }
    }

}
