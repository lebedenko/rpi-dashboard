pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: root

    required property var deviceModel
    required property int selectedIndex
    signal selectionRequested(int index)
    readonly property var selectedDevice: root.selectedIndex >= 0 && root.selectedIndex < root.deviceModel.count
                                          ? root.deviceModel.get(root.selectedIndex) : null
    property Item focusTarget: null
    onSelectedIndexChanged: root.focusTarget = deviceTabs.itemAt(root.selectedIndex)

    function availableNumber(value): bool { return value !== undefined && value !== null && Number(value) >= 0 }
    function ratio(value): real { return root.availableNumber(value) ? Number(value) : -1 }
    function percent(value): string { return root.availableNumber(value) ? qsTr("%1%").arg(Math.round(Number(value) * 100)) : "—" }
    function frequency(value): string {
        return root.availableNumber(value) ? qsTr("%1 GHz").arg((Number(value) / 1000000000).toFixed(2)) : "—"
    }
    function bytes(value): string {
        if (!root.availableNumber(value)) return "—"
        const bytesValue = Number(value)
        return bytesValue >= 1048576 ? qsTr("%1 MiB").arg((bytesValue / 1048576).toFixed(0))
                                     : qsTr("%1 KiB").arg((bytesValue / 1024).toFixed(0))
    }
    function rate(value): string {
        if (!root.availableNumber(value)) return "—"
        const bytesValue = Number(value)
        return bytesValue >= 1048576 ? qsTr("%1 MiB/s").arg((bytesValue / 1048576).toFixed(1))
                                     : qsTr("%1 KiB/s").arg((bytesValue / 1024).toFixed(1))
    }
    function temperature(value): string { return root.availableNumber(value) ? qsTr("%1°C").arg(Math.round(Number(value))) : "—" }
    function uptime(value): string {
        if (!root.availableNumber(value)) return "—"
        const minutes = Math.floor(Number(value) / 60)
        const hours = Math.floor(minutes / 60)
        const days = Math.floor(hours / 24)
        return days > 0 ? qsTr("%1d %2h").arg(days).arg(hours % 24)
                        : hours > 0 ? qsTr("%1h %2m").arg(hours).arg(minutes % 60)
                                    : qsTr("%1m").arg(minutes)
    }
    function bootTime(value): string {
        return root.availableNumber(value) ? Qt.formatDateTime(new Date(Number(value)), qsTr("ddd hh:mm")) : "—"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.displaySafeInset
        spacing: Theme.spacingSmall

        Row {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.touchTarget
            Layout.minimumHeight: Theme.touchTarget
            Layout.maximumHeight: Theme.touchTarget
            spacing: Theme.spacingSmall

            Repeater {
                id: deviceTabs
                model: root.deviceModel
                onItemAdded: (index, item) => {
                    if (index === root.selectedIndex)
                        root.focusTarget = item
                }
                delegate: Button {
                    id: tab
                    required property int index
                    required property string deviceNumber
                    required property string hostname

                    objectName: "systemDeviceTab" + tab.index
                    width: 132
                    height: Theme.touchTarget
                    activeFocusOnTab: true
                    text: qsTr("%1  %2").arg(tab.deviceNumber).arg(tab.hostname)
                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Select device %1").arg(tab.hostname)
                    onClicked: root.selectionRequested(tab.index)
                    Keys.onSpacePressed: root.selectionRequested(tab.index)
                    Keys.onReturnPressed: root.selectionRequested(tab.index)
                    Keys.onEnterPressed: root.selectionRequested(tab.index)

                    contentItem: Label {
                        text: tab.text
                        color: root.selectedIndex === tab.index ? Theme.textPrimary : Theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.family: Theme.fixedFontFamily
                        font.pixelSize: Theme.bodyTextSize
                        font.weight: Theme.technicalFontWeight
                    }
                    background: Rectangle {
                        color: root.selectedIndex === tab.index ? Theme.selectedSurface : Theme.surface
                        border.width: tab.activeFocus ? 2 : 1
                        border.color: tab.activeFocus ? Theme.focusAccent
                                                          : root.selectedIndex === tab.index ? Theme.primaryAccent : Theme.passiveBorder
                        radius: 4
                    }
                }
            }
        }

        RowLayout {
            id: panels
            objectName: "systemPanels"
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6

            SystemMetricPanel {
                objectName: "cpuPanel"
                Layout.fillWidth: true; Layout.fillHeight: true
                heading: qsTr("CPU")
                primaryValue: root.percent(root.selectedDevice ? root.selectedDevice.cpuUsageRatio : -1)
                secondaryLabel: qsTr("FREQ")
                secondaryValue: root.frequency(root.selectedDevice ? root.selectedDevice.cpuFrequencyHz : -1)
                usageRatio: root.ratio(root.selectedDevice ? root.selectedDevice.cpuUsageRatio : -1)
                accentColor: Theme.cpuSeries
            }
            SystemMetricPanel {
                objectName: "gpuPanel"
                Layout.fillWidth: true; Layout.fillHeight: true
                heading: qsTr("GPU")
                primaryValue: root.percent(root.selectedDevice ? root.selectedDevice.gpuUsageRatio : -1)
                secondaryLabel: qsTr("CORE")
                secondaryValue: root.frequency(root.selectedDevice ? root.selectedDevice.gpuCoreClockHz : -1)
                usageRatio: root.ratio(root.selectedDevice ? root.selectedDevice.gpuUsageRatio : -1)
                accentColor: Theme.violetAccent
            }
            SystemMetricPanel {
                objectName: "memoryPanel"
                Layout.fillWidth: true; Layout.fillHeight: true
                heading: qsTr("MEMORY")
                primaryValue: root.bytes(root.selectedDevice ? root.selectedDevice.memoryUsedBytes : -1)
                secondaryLabel: qsTr("SWAP")
                secondaryValue: root.bytes(root.selectedDevice ? root.selectedDevice.swapUsedBytes : -1)
                usageRatio: root.ratio(root.selectedDevice ? root.selectedDevice.memoryUsageRatio : -1)
                accentColor: Theme.memorySeries
            }
            SystemMetricPanel {
                objectName: "thermalsPanel"
                Layout.fillWidth: true; Layout.fillHeight: true
                heading: qsTr("THERMALS")
                primaryValue: root.temperature(root.selectedDevice ? root.selectedDevice.boardTemperatureCelsius : -1)
                secondaryLabel: qsTr("GPU")
                secondaryValue: root.temperature(root.selectedDevice ? root.selectedDevice.gpuTemperatureCelsius : -1)
                accentColor: Theme.attentionStatus
            }
            SystemMetricPanel {
                objectName: "networkPanel"
                Layout.fillWidth: true; Layout.fillHeight: true
                heading: root.selectedDevice && root.selectedDevice.networkInterfaceName.length > 0
                         ? qsTr("NET %1").arg(root.selectedDevice.networkInterfaceName) : qsTr("NETWORK")
                primaryValue: qsTr("↓ %1").arg(root.rate(root.selectedDevice ? root.selectedDevice.networkReceiveRate : -1))
                secondaryLabel: qsTr("UP")
                secondaryValue: root.rate(root.selectedDevice ? root.selectedDevice.networkTransmitRate : -1)
                accentColor: Theme.onlineStatus
            }
            SystemMetricPanel {
                objectName: "uptimePanel"
                Layout.fillWidth: true; Layout.fillHeight: true
                heading: qsTr("UPTIME")
                primaryValue: root.uptime(root.selectedDevice ? root.selectedDevice.uptimeSeconds : -1)
                secondaryLabel: qsTr("BOOT")
                secondaryValue: root.bootTime(root.selectedDevice ? root.selectedDevice.bootTimeMs : -1)
                accentColor: Theme.primaryAccent
            }
        }
    }

    function focusSelected(): void {
        const tab = deviceTabs.itemAt(root.selectedIndex)
        if (tab) tab.forceActiveFocus()
    }
}
