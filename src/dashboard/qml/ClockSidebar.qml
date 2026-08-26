import QtQuick
import QtQuick.Shapes

Item {
    id: root

    property date currentTimestamp: new Date()
    property var projectsService: null
    readonly property string timeText: Qt.formatTime(root.currentTimestamp, "hh:mm")
    readonly property string dateText: Qt.formatDate(root.currentTimestamp, "ddd dd MMM")
    readonly property string ciText: qsTr("%1 CI").arg(root.statusLabel(root.projectsService ? root.projectsService.aggregateHealth : "unknown"))
    readonly property string runnerText: root.projectsService && root.projectsService.totalRunnerCount >= 0
                                         ? qsTr("%1/%2 RUNNERS").arg(root.projectsService.onlineRunnerCount).arg(root.projectsService.totalRunnerCount)
                                         : qsTr("— RUNNERS")
    readonly property string accessibleText: qsTr("%1, %2, %3, %4").arg(root.timeText).arg(root.dateText).arg(root.ciText).arg(root.runnerText)

    objectName: "clockSidebar"
    Accessible.role: Accessible.StaticText
    Accessible.name: root.accessibleText

    Shape {
        id: sidebarBackground

        anchors.fill: parent
        Accessible.ignored: true

        ShapePath {
            fillColor: Theme.surface
            strokeColor: Theme.passiveBorder
            strokeWidth: 1
            joinStyle: ShapePath.MiterJoin
            startX: Theme.sidebarCornerRadius
            startY: 0
            PathLine { x: sidebarBackground.width - Theme.sidebarChamfer; y: 0 }
            PathLine { x: sidebarBackground.width; y: Theme.sidebarChamfer }
            PathLine { x: sidebarBackground.width; y: sidebarBackground.height - Theme.sidebarChamfer }
            PathLine { x: sidebarBackground.width - Theme.sidebarChamfer; y: sidebarBackground.height }
            PathLine { x: Theme.sidebarChamfer; y: sidebarBackground.height }
            PathLine { x: 0; y: sidebarBackground.height - Theme.sidebarChamfer }
            PathLine { x: 0; y: Theme.sidebarCornerRadius }
            PathArc {
                x: Theme.sidebarCornerRadius
                y: 0
                radiusX: Theme.sidebarCornerRadius
                radiusY: Theme.sidebarCornerRadius
            }
        }
    }

    Column {
        anchors.top: parent.top
        anchors.topMargin: Theme.spacingSmall
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.spacingSmall

        Text {
            id: timeLabel

            objectName: "clockTimeLabel"
            width: parent.width
            color: Theme.primaryAccent
            font.family: Theme.sansFontFamily
            font.pixelSize: Theme.clockTimeTextSize
            font.weight: Theme.metricFontWeight
            horizontalAlignment: Text.AlignHCenter
            text: root.timeText
            Accessible.ignored: true
        }

        Text {
            id: dateLabel

            objectName: "clockDateLabel"
            width: parent.width
            color: Theme.violetAccent
            elide: Text.ElideRight
            font.family: Theme.sansFontFamily
            font.pixelSize: Theme.clockDateTextSize
            font.weight: Theme.informationFontWeight
            horizontalAlignment: Text.AlignHCenter
            text: root.dateText
            Accessible.ignored: true
        }

        Rectangle {
            width: parent.width - 16
            height: 1
            anchors.horizontalCenter: parent.horizontalCenter
            color: Theme.sectionDivider
            Accessible.ignored: true
        }

        Text {
            objectName: "globalCiHealth"
            width: parent.width
            color: root.statusColor(root.projectsService ? root.projectsService.aggregateHealth : "unknown")
            font.family: Theme.sansFontFamily
            font.pixelSize: Theme.sectionTitleTextSize
            font.weight: Theme.technicalFontWeight
            horizontalAlignment: Text.AlignHCenter
            text: root.ciText
            Accessible.ignored: true
        }

        Text {
            objectName: "globalRunnerHealth"
            width: parent.width
            color: Theme.textSecondary
            font.family: Theme.fixedFontFamily
            font.pixelSize: Theme.captionTextSize
            horizontalAlignment: Text.AlignHCenter
            text: root.runnerText
            Accessible.ignored: true
        }
    }

    function statusLabel(health): string {
        const labels = {"failed": qsTr("FAILED"), "attention": qsTr("ATTENTION"),
                        "running": qsTr("RUNNING"), "stale": qsTr("STALE"),
                        "healthy": qsTr("HEALTHY"), "unknown": qsTr("UNKNOWN")}
        return labels[health] || labels.unknown
    }

    function statusColor(health): color {
        const colors = {"failed": Theme.failureStatus, "attention": Theme.attentionStatus,
                        "running": Theme.runningStatus, "stale": Theme.staleStatus,
                        "healthy": Theme.healthyStatus, "unknown": Theme.unknownStatus}
        return colors[health] || Theme.unknownStatus
    }

    Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: root.currentTimestamp = new Date()
    }
}
