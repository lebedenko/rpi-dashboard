import QtQuick
import QtQuick.Shapes

Item {
    id: root

    property date currentTimestamp: new Date()
    property var projectsService: null
    property var weatherService: null
    property bool weatherMode: false
    readonly property string timeText: Qt.formatTime(root.currentTimestamp, "hh:mm")
    readonly property string dateText: Qt.formatDate(root.currentTimestamp, "ddd dd MMM")
    readonly property string ciText: qsTr("%1 CI").arg(root.statusLabel(root.projectsService ? root.projectsService.aggregateHealth : "unknown"))
    readonly property string runnerText: root.projectsService && root.projectsService.totalRunnerCount >= 0
                                         ? qsTr("%1/%2 RUNNERS").arg(root.projectsService.onlineRunnerCount).arg(root.projectsService.totalRunnerCount)
                                         : qsTr("— RUNNERS")
    readonly property string accessibleText: root.weatherMode
        ? qsTr("%1, %2, air quality %3, sunset %4, rain probability %5 percent")
            .arg(root.timeText).arg(root.dateText).arg(root.weatherService ? root.weatherService.airQualityCategory : qsTr("unavailable"))
            .arg(root.weatherService ? root.weatherService.localSunset : "—")
            .arg(root.weatherService ? Math.round(root.weatherService.todayRainProbabilityPercent) : 0)
        : qsTr("%1, %2, %3, %4").arg(root.timeText).arg(root.dateText).arg(root.ciText).arg(root.runnerText)

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
            startX: Theme.radiusMedium
            startY: 0
            PathLine { x: sidebarBackground.width - Theme.chamferLarge; y: 0 }
            PathLine { x: sidebarBackground.width; y: Theme.chamferLarge }
            PathLine { x: sidebarBackground.width; y: sidebarBackground.height - Theme.chamferLarge }
            PathLine { x: sidebarBackground.width - Theme.chamferLarge; y: sidebarBackground.height }
            PathLine { x: Theme.chamferLarge; y: sidebarBackground.height }
            PathLine { x: 0; y: sidebarBackground.height - Theme.chamferLarge }
            PathLine { x: 0; y: Theme.radiusMedium }
            PathArc {
                x: Theme.radiusMedium
                y: 0
                radiusX: Theme.radiusMedium
                radiusY: Theme.radiusMedium
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
            width: parent.width - Theme.spacingMedium
            height: 1
            anchors.horizontalCenter: parent.horizontalCenter
            color: Theme.sectionDivider
            Accessible.ignored: true
        }

        Text {
            objectName: "globalCiHealth"
            width: parent.width
            color: Theme.statusColor(root.projectsService ? root.projectsService.aggregateHealth : "unknown")
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
            visible: !root.weatherMode
            Accessible.ignored: true
        }

        Column {
            objectName: "weatherRail"
            width: parent.width
            spacing: Theme.spacingSmall
            visible: root.weatherMode
            Text { width: parent.width; color: Theme.textMuted; horizontalAlignment: Text.AlignHCenter; font.family: Theme.sansFontFamily; font.pixelSize: Theme.captionTextSize; text: qsTr("AIR QUALITY") }
            Text { objectName: "weatherAqi"; width: parent.width; color: Theme.primaryAccent; horizontalAlignment: Text.AlignHCenter; font.family: Theme.fixedFontFamily; font.pixelSize: Theme.metricTextSize; text: root.weatherService && root.weatherService.airQualityIndex ? qsTr("%1 · %2").arg(root.weatherService.airQualityIndex).arg(root.weatherService.airQualityCategory) : "—" }
            Text { width: parent.width; color: Theme.textMuted; horizontalAlignment: Text.AlignHCenter; font.family: Theme.sansFontFamily; font.pixelSize: Theme.captionTextSize; text: qsTr("SUNSET") }
            Text { objectName: "weatherSunset"; width: parent.width; color: Theme.attentionStatus; horizontalAlignment: Text.AlignHCenter; font.family: Theme.fixedFontFamily; font.pixelSize: Theme.bodyTextSize; text: root.weatherService && root.weatherService.localSunset ? root.weatherService.localSunset : "—" }
            Text { width: parent.width; color: Theme.textMuted; horizontalAlignment: Text.AlignHCenter; font.family: Theme.sansFontFamily; font.pixelSize: Theme.captionTextSize; text: qsTr("RAIN TODAY") }
            Text { objectName: "weatherRain"; width: parent.width; color: Theme.violetAccent; horizontalAlignment: Text.AlignHCenter; font.family: Theme.fixedFontFamily; font.pixelSize: Theme.bodyTextSize; text: root.weatherService ? qsTr("%1%").arg(Math.round(root.weatherService.todayRainProbabilityPercent)) : "—" }
            Item {
                width: parent.width
                height: Theme.touchTarget
                activeFocusOnTab: true
                Accessible.role: Accessible.Link
                Accessible.name: qsTr("OpenWeather weather data")
                Text { anchors.centerIn: parent; color: Theme.textMuted; font.family: Theme.sansFontFamily; font.pixelSize: Theme.axisTextSize; text: qsTr("OpenWeather") }
                TapHandler { onTapped: Qt.openUrlExternally("https://openweathermap.org/") }
                Keys.onReturnPressed: Qt.openUrlExternally("https://openweathermap.org/")
                Keys.onSpacePressed: Qt.openUrlExternally("https://openweathermap.org/")
            }
        }
    }

    function statusLabel(health): string {
        const labels = {"failed": qsTr("FAILED"), "attention": qsTr("ATTENTION"),
                        "running": qsTr("RUNNING"), "stale": qsTr("STALE"),
                        "healthy": qsTr("HEALTHY"), "unknown": qsTr("UNKNOWN")}
        return labels[health] || labels.unknown
    }

    Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: root.currentTimestamp = new Date()
    }
}
