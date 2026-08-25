pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Shapes

ApplicationWindow {
    id: root

    property bool windowed: false
    property int windowWidth: 1480
    property int windowHeight: 320
    property var sysInfoService: null
    property var sysMetricsService: null
    readonly property alias currentPageIndex: pageStack.currentIndex
    readonly property bool currentPageHasFocus: root.currentFocusTarget ? root.currentFocusTarget.activeFocus : false

    readonly property var currentFocusTarget: pageStack.currentIndex === 0 ? overviewPage.focusTarget
                                               : pageStack.currentIndex === 1 ? systemsPage.placeholder
                                               : pageStack.currentIndex === 2 ? projectsPage.placeholder
                                               : weatherPage.placeholder
    readonly property alias localDeviceModel: localDevices

    width: root.windowWidth
    height: root.windowHeight
    visibility: root.windowed ? Window.Windowed : Window.FullScreen
    color: Theme.background
    title: qsTr("HoloNight Dashboard")

    function selectPreviousPage(): void {
        pageStack.currentIndex = Math.max(0, pageStack.currentIndex - 1)
    }

    function selectNextPage(): void {
        pageStack.currentIndex = Math.min(pageStack.count - 1, pageStack.currentIndex + 1)
    }

    function isAvailable(value): bool {
        return value !== undefined && value !== null && String(value).length > 0
    }

    function valueOrPlaceholder(value): string {
        return root.isAvailable(value) ? String(value) : "—"
    }

    function combine(first, second): string {
        const values = [first, second].filter(value => root.isAvailable(value))
        return values.length > 0 ? values.join(" · ") : "—"
    }

    function formatCores(physical, logical): string {
        if (root.isAvailable(physical) && root.isAvailable(logical))
            return qsTr("%1 physical · %2 logical").arg(physical).arg(logical)
        if (root.isAvailable(logical))
            return qsTr("%1 logical").arg(logical)
        if (root.isAvailable(physical))
            return qsTr("%1 physical").arg(physical)
        return "—"
    }

    function formatBytes(bytes): string {
        if (!root.isAvailable(bytes))
            return "—"
        return qsTr("%1 GiB").arg((Number(bytes) / 1073741824).toFixed(1))
    }

    function formatPercentage(ratio): string {
        return root.isAvailable(ratio) ? qsTr("%1%").arg(Math.round(Number(ratio) * 100)) : "—"
    }

    function usageRatioOrUnavailable(ratio): real {
        return root.isAvailable(ratio) ? Number(ratio) : -1
    }

    function formatTemperature(celsius): string {
        return root.isAvailable(celsius) ? qsTr("%1°C").arg(Math.round(Number(celsius))) : "—"
    }

    function formatUptime(seconds): string {
        if (!root.isAvailable(seconds))
            return "—"
        const minutes = Math.max(0, Math.floor(Number(seconds) / 60))
        if (minutes < 1)
            return qsTr("<1m")
        const hours = Math.floor(minutes / 60)
        const days = Math.floor(hours / 24)
        if (days > 0)
            return qsTr("%1d %2h").arg(days).arg(hours % 24)
        if (hours > 0)
            return qsTr("%1h %2m").arg(hours).arg(minutes % 60)
        return qsTr("%1m").arg(minutes)
    }

    function refreshLocalMetrics(): void {
        const service = root.sysMetricsService
        localDevices.setProperty(0, "cpuMetric", root.formatPercentage(service ? service.cpuUsageRatio : undefined))
        localDevices.setProperty(0, "memoryMetric", root.formatPercentage(service ? service.memoryUsageRatio : undefined))
        localDevices.setProperty(0, "cpuUsageRatio", root.usageRatioOrUnavailable(service ? service.cpuUsageRatio : undefined))
        localDevices.setProperty(0, "memoryUsageRatio", root.usageRatioOrUnavailable(service ? service.memoryUsageRatio : undefined))
        localDevices.setProperty(0, "temperatureMetric", root.formatTemperature(service ? service.cpuTemperatureCelsius : undefined))
        localDevices.setProperty(0, "uptimeMetric", root.formatUptime(service ? service.uptimeSeconds : undefined))
    }

    function refreshLocalDevice(): void {
        const service = root.sysInfoService
        localDevices.setProperty(0, "hostname", root.valueOrPlaceholder(service ? service.hostname : undefined).toUpperCase())
        localDevices.setProperty(0, "osDescription", root.combine(service ? service.osPrettyName : undefined,
                                                                  service ? service.osVersion : undefined))
        localDevices.setProperty(0, "kernelDescription", root.combine(service ? service.kernelType : undefined,
                                                                      service ? service.kernelVersion : undefined))
        localDevices.setProperty(0, "architecture", root.valueOrPlaceholder(service ? service.architecture : undefined))
        localDevices.setProperty(0, "hardwareDescription", root.combine(service ? service.hardwareManufacturer : undefined,
                                                                        service ? service.hardwareModel : undefined))
        localDevices.setProperty(0, "cpuDescription", root.combine(service ? service.cpuVendor : undefined,
                                                                   service ? service.cpuModel : undefined))
        localDevices.setProperty(0, "coreDescription", root.formatCores(service ? service.physicalCoreCount : undefined,
                                                                        service ? service.logicalCpuCount : undefined))
        localDevices.setProperty(0, "totalMemory", root.formatBytes(service ? service.totalMemoryBytes : undefined))
    }

    ListModel {
        id: localDevices
        ListElement {
            deviceNumber: "01"
            hostname: "—"
            online: true
            cpuMetric: "—"
            memoryMetric: "—"
            cpuUsageRatio: -1
            memoryUsageRatio: -1
            temperatureMetric: "—"
            uptimeMetric: "—"
            osDescription: "—"
            kernelDescription: "—"
            architecture: "—"
            hardwareDescription: "—"
            cpuDescription: "—"
            coreDescription: "—"
            totalMemory: "—"
        }
    }

    Connections {
        target: root.sysInfoService
        ignoreUnknownSignals: true
        function onHostnameChanged(): void { root.refreshLocalDevice() }
        function onOsFamilyChanged(): void { root.refreshLocalDevice() }
        function onOsIdChanged(): void { root.refreshLocalDevice() }
        function onOsVersionChanged(): void { root.refreshLocalDevice() }
        function onOsPrettyNameChanged(): void { root.refreshLocalDevice() }
        function onKernelTypeChanged(): void { root.refreshLocalDevice() }
        function onKernelVersionChanged(): void { root.refreshLocalDevice() }
        function onArchitectureChanged(): void { root.refreshLocalDevice() }
        function onHardwareManufacturerChanged(): void { root.refreshLocalDevice() }
        function onHardwareModelChanged(): void { root.refreshLocalDevice() }
        function onCpuVendorChanged(): void { root.refreshLocalDevice() }
        function onCpuModelChanged(): void { root.refreshLocalDevice() }
        function onPhysicalCoreCountChanged(): void { root.refreshLocalDevice() }
        function onLogicalCpuCountChanged(): void { root.refreshLocalDevice() }
        function onTotalMemoryBytesChanged(): void { root.refreshLocalDevice() }
    }

    Connections {
        target: root.sysMetricsService
        ignoreUnknownSignals: true
        function onCurrentMetricsChanged(): void { root.refreshLocalMetrics() }
    }

    Component.onCompleted: {
        root.refreshLocalDevice()
        root.refreshLocalMetrics()
    }

    Shortcut { sequence: "Home"; onActivated: pageStack.currentIndex = 0 }
    Shortcut { sequence: "Left"; onActivated: root.selectPreviousPage() }
    Shortcut { sequence: "Right"; onActivated: root.selectNextPage() }
    Shortcut { sequence: "F5"; onActivated: { if (root.currentFocusTarget) root.currentFocusTarget.forceActiveFocus() } }
    Shortcut { sequence: "Ctrl+Q"; onActivated: root.close() }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            id: sidebarSurface
            objectName: "sidebarSurface"

            Layout.preferredWidth: Theme.sidebarWidth
            Layout.fillHeight: true
            Layout.leftMargin: Theme.displaySafeInset
            Layout.topMargin: Theme.displaySafeInset
            Layout.bottomMargin: Theme.displaySafeInset

            Shape {
                id: sidebarBackground

                anchors.fill: parent
                Accessible.ignored: true

                ShapePath {
                    fillColor: Theme.surface
                    strokeColor: Theme.passiveBorder
                    strokeWidth: 1
                    joinStyle: ShapePath.MiterJoin
                    startX: Theme.sidebarChamfer
                    startY: 0
                    PathLine { x: sidebarBackground.width - Theme.sidebarCornerRadius; y: 0 }
                    PathArc {
                        x: sidebarBackground.width
                        y: Theme.sidebarCornerRadius
                        radiusX: Theme.sidebarCornerRadius
                        radiusY: Theme.sidebarCornerRadius
                    }
                    PathLine { x: sidebarBackground.width; y: sidebarBackground.height - Theme.sidebarChamfer }
                    PathLine { x: sidebarBackground.width - Theme.sidebarChamfer; y: sidebarBackground.height }
                    PathLine { x: Theme.sidebarChamfer; y: sidebarBackground.height }
                    PathLine { x: 0; y: sidebarBackground.height - Theme.sidebarChamfer }
                    PathLine { x: 0; y: Theme.sidebarChamfer }
                    PathLine { x: Theme.sidebarChamfer; y: 0 }
                }
            }

            Column {
                anchors.top: parent.top
                anchors.topMargin: Theme.spacingSmall
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.spacingSmall

                SidebarButton {
                    objectName: "overviewButton"
                    width: Theme.touchTarget
                    height: Theme.touchTarget
                    selected: pageStack.currentIndex === 0
                    iconSource: Qt.resolvedUrl("icons/overview.svg")
                    tooltipText: qsTr("Overview")
                    onClicked: pageStack.currentIndex = 0
                }

                SidebarButton {
                    objectName: "systemsButton"
                    width: Theme.touchTarget
                    height: Theme.touchTarget
                    selected: pageStack.currentIndex === 1
                    iconSource: Qt.resolvedUrl("icons/systems.svg")
                    tooltipText: qsTr("Systems")
                    onClicked: pageStack.currentIndex = 1
                }

                SidebarButton {
                    objectName: "projectsButton"
                    width: Theme.touchTarget
                    height: Theme.touchTarget
                    selected: pageStack.currentIndex === 2
                    iconSource: Qt.resolvedUrl("icons/projects.svg")
                    tooltipText: qsTr("Projects")
                    onClicked: pageStack.currentIndex = 2
                }

                SidebarButton {
                    objectName: "weatherButton"
                    width: Theme.touchTarget
                    height: Theme.touchTarget
                    selected: pageStack.currentIndex === 3
                    iconSource: Qt.resolvedUrl("icons/weather.svg")
                    tooltipText: qsTr("Weather")
                    onClicked: pageStack.currentIndex = 3
                }
            }
        }

        StackLayout {
            id: pageStack
            objectName: "pageStack"

            currentIndex: 0
            Layout.fillWidth: true
            Layout.fillHeight: true

            OverviewPage {
                id: overviewPage
                objectName: "overviewPage"
                deviceModel: root.localDeviceModel
                usageHistoryModel: root.sysMetricsService ? root.sysMetricsService.usageHistoryModel : null
            }
            DashboardPage { id: systemsPage; objectName: "systemsPage"; heading: qsTr("Systems") }
            DashboardPage { id: projectsPage; objectName: "projectsPage"; heading: qsTr("Projects") }
            DashboardPage { id: weatherPage; objectName: "weatherPage"; heading: qsTr("Weather") }
        }

        ClockSidebar {
            Layout.preferredWidth: Theme.statusSidebarWidth
            Layout.fillHeight: true
            Layout.topMargin: Theme.displaySafeInset
            Layout.rightMargin: Theme.displaySafeInset
            Layout.bottomMargin: Theme.displaySafeInset
        }
    }
}
