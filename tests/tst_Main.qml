import HoloNight.Dashboard
import QtQuick
import QtTest

Item {
    id: root

    function deviceEntry(number, name, selected = true, cpuMetric = "—", memoryMetric = "—", temperatureMetric = "—", uptimeMetric = "—") {
        return {
            "deviceNumber": number,
            "hostname": name,
            "online": true,
            "selected": selected,
            "cpuMetric": cpuMetric,
            "memoryMetric": memoryMetric,
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

        function init() {
            fakeService = fakeServiceComponent.createObject(root);
            dashboardComponent = Qt.createComponent("qrc:/qt/qml/HoloNight/Dashboard/qml/Main.qml");
            verify(dashboardComponent.status === Component.Ready, dashboardComponent.errorString());
            dashboardWindow = dashboardComponent.createObject(null, {
                "windowed": true,
                "windowWidth": 1480,
                "windowHeight": 320,
                "sysInfoService": fakeService
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
        }

        function test_exactDevelopmentGeometry() {
            compare(dashboardWindow.width, 1480);
            compare(dashboardWindow.height, 320);
            compare(dashboardWindow.visibility, Window.Windowed);
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

        function test_touchTargetsSelectPages() {
            const buttonNames = ["overviewButton", "systemsButton", "projectsButton", "weatherButton"];
            for (let index = 0; index < buttonNames.length; ++index) {
                const button = findChild(dashboardWindow.contentItem, buttonNames[index]);
                verify(!!button, "Object exists");
                compare(button.width, 56);
                compare(button.height, 56);
                mouseClick(button);
                tryCompare(dashboardWindow, "currentPageIndex", index);
            }
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
            compare(card.cpuMetric, "—");
            compare(card.memoryMetric, "—");
            compare(card.temperatureMetric, "—");
            compare(card.uptimeMetric, "—");
            compare(card.osDescription, "Raspberry Pi OS · 13");
            compare(card.kernelDescription, "linux · 6.12-test");
            compare(card.architecture, "aarch64");
            compare(card.hardwareDescription, "Raspberry Pi · Raspberry Pi 5 Model B");
            compare(card.cpuDescription, "Broadcom · BCM2712");
            compare(card.coreDescription, "4 physical · 4 logical");
            compare(card.totalMemory, "8.0 GiB");
            compare(card.expanded, true);
            compare(card.height, 298);
            tryCompare(card, "expandedContentLoaded", true);
            compare(card.chevronButton.width, 48);
            compare(card.chevronButton.height, 48);
            compare(card.chevronAccessibleName, "Collapse PI-DASH");
            const footer = findChild(card, "deviceFooter");
            verify(!!footer);
            compare(findChild(card, "selectActiveButton").checked, true);
            compare(findChild(card, "viewStreamsButton").enabled, false);
            compare(findChild(card, "terminalButton").enabled, false);
            compare(findChild(card, "moreButton").enabled, false);
            verify(!!findChild(card, "resourceHistory"));
            verify(!!findChild(card, "historyGrid"));
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

        function test_selectedActionRemainsModelBoundAndIdempotent() {
            const overview = findChild(dashboardWindow.contentItem, "overviewPage");
            const card = findChild(overview, "deviceCard0");
            const selectButton = findChild(card, "selectActiveButton");
            compare(card.selected, true);
            compare(selectButton.checked, true);
            mouseClick(selectButton);
            compare(card.selected, true);
            compare(selectButton.checked, true);
            overview.deviceModel.setProperty(0, "selected", false);
            tryCompare(card, "selected", false);
            tryCompare(selectButton, "checked", false);
        }

        function test_metricAndSelectionRolesAreForwarded() {
            const overview = findChild(dashboardWindow.contentItem, "overviewPage");
            overview.deviceModel = listModelComponent.createObject(root);
            overview.deviceModel.append(deviceEntry("01", "FORWARDED", false, "17%", "33%", "42°C", "2h"));
            tryCompare(overview.deviceList, "count", 1);
            const card = findChild(overview, "deviceCard0");
            tryCompare(card, "hostname", "FORWARDED");
            compare(card.selected, false);
            compare(card.cpuMetric, "17%");
            compare(card.memoryMetric, "33%");
            compare(card.temperatureMetric, "42°C");
            compare(card.uptimeMetric, "2h");
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

        function test_deviceCardGeometryMatchesFixedViewportComposition() {
            const card = findChild(dashboardWindow.contentItem, "deviceCard0");
            const footer = findChild(card, "deviceFooter");
            const details = findChild(card, "deviceDetails");
            const history = findChild(card, "resourceHistory");
            verify(!!footer);
            verify(!!details);
            verify(!!history);
            compare(Math.abs(details.width / (details.width + history.width) - 0.32) < 0.025, true);
            const footerButtons = [findChild(card, "selectActiveButton"), findChild(card, "viewStreamsButton"), findChild(card, "terminalButton"), findChild(card, "moreButton")];
            compare(footer.x, 0);
            compare(footer.width, card.availableWidth);
            for (let index = 1; index < footerButtons.length; ++index) compare(Math.abs(footerButtons[index].width - footerButtons[0].width) <= 1, true)
            compare(footerButtons[0].selectedSurfaceVisible, true);
            for (let index = 1; index < footerButtons.length; ++index) compare(footerButtons[index].disabledSurfaceVisible, true)
            const firstRow = findChild(card, "detailRow0");
            const lastRow = findChild(card, "detailRow6");
            verify(!!firstRow);
            verify(!!lastRow);
            compare(firstRow.valuePixelSize >= 14, true);
            compare(lastRow.y + lastRow.height <= details.height, true);
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

}
