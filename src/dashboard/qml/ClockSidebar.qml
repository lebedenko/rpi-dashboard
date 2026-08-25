import QtQuick
import QtQuick.Shapes

Item {
    id: root

    property date currentTimestamp: new Date()
    readonly property string timeText: Qt.formatTime(root.currentTimestamp, Locale.ShortFormat)
    readonly property string dateText: Qt.formatDate(root.currentTimestamp, Locale.ShortFormat)
    readonly property string accessibleText: qsTr("%1, %2").arg(root.timeText).arg(root.dateText)

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
        anchors.topMargin: Theme.spacingMedium
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.spacingSmall

        Text {
            id: timeLabel

            objectName: "clockTimeLabel"
            width: parent.width
            color: Theme.textPrimary
            font.family: Theme.fixedFontFamily
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
            color: Theme.textSecondary
            elide: Text.ElideRight
            font.family: Theme.sansFontFamily
            font.pixelSize: Theme.clockDateTextSize
            font.weight: Theme.informationFontWeight
            horizontalAlignment: Text.AlignHCenter
            text: root.dateText
            Accessible.ignored: true
        }
    }

    Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: root.currentTimestamp = new Date()
    }
}
