pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Rpi.Dashboard as Dashboard

FocusScope {
    id: root

    property var service: null
    readonly property alias focusTarget: refreshArea
    readonly property string stateText: !root.service ? qsTr("Weather unavailable")
                                        : root.service.state === "unconfigured" ? qsTr("Weather is not configured")
                                        : root.service.state === "locating" ? qsTr("Finding location…")
                                        : root.service.state === "loading" ? qsTr("Updating weather…")
                                        : root.service.diagnostics
    readonly property string locationText: root.service && root.service.city
                                                   ? qsTr("%1, %2").arg(root.service.city).arg(root.service.country)
                                                   : qsTr("Location unavailable")
    readonly property string accessibleSummary: root.service && root.service.state === "ready"
                                                ? qsTr("Weather for %1. %2, %3 degrees Celsius. Humidity %4 percent. Wind %5 kilometres per hour.")
                                                    .arg(root.locationText).arg(root.service.condition)
                                                    .arg(Math.round(root.service.temperatureCelsius))
                                                    .arg(Math.round(root.service.humidityPercent))
                                                    .arg(Math.round(root.service.windSpeedKmh))
                                                : root.stateText

    objectName: "weatherPage"
    Accessible.role: Accessible.Pane
    Accessible.name: root.accessibleSummary

    function iconUrl(code): string {
        return "image://weather/" + (code || "03d") + "/8295ac/ffc857/a66cff"
    }

    function temperature(value): string { return qsTr("%1°").arg(Math.round(Number(value))) }

    Dashboard.Frame {
        objectName: "weatherPageFrame"
        anchors.fill: parent
        anchors.margins: Theme.spacingSmall
        backgroundColor: Theme.cardSurface
        color: Theme.cardFrame
        lineWidth: 1
        corners: ({ rounded: Theme.radiusMedium })
        Accessible.ignored: true
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingMedium
        spacing: Theme.spacingSmall

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            spacing: Theme.spacingSmall

            Text {
                objectName: "weatherHeading"
                Layout.preferredWidth: 100
                color: Theme.primaryAccent
                font.family: Theme.sansFontFamily
                font.pixelSize: Theme.headingTextSize
                font.weight: Theme.headingFontWeight
                text: qsTr("WEATHER")
            }
            Text {
                objectName: "weatherLocation"
                Layout.fillWidth: true
                color: Theme.textSecondary
                elide: Text.ElideRight
                font.family: Theme.sansFontFamily
                font.pixelSize: Theme.bodyTextSize
                text: root.locationText
            }
            Text {
                objectName: "weatherStatus"
                Layout.preferredWidth: 240
                color: root.service && (root.service.stale || root.service.state === "error")
                       ? Theme.attentionStatus : Theme.textMuted
                elide: Text.ElideRight
                font.family: Theme.fixedFontFamily
                font.pixelSize: Theme.captionTextSize
                horizontalAlignment: Text.AlignRight
                text: root.service && root.service.lastSuccessUtc
                      ? qsTr("UPDATED %1%2").arg(Qt.formatTime(root.service.lastSuccessUtc, "hh:mm"))
                                              .arg(root.service.stale ? qsTr(" · STALE") : "")
                      : root.stateText
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingSmall

            Dashboard.Frame {
                objectName: "currentConditionsPanel"
                Layout.preferredWidth: 285
                Layout.fillHeight: true
                backgroundColor: Theme.surface
                color: Theme.sectionDividerStrong
                lineWidth: 1
                corners: ({ rounded: Theme.radiusMedium })

                Row {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingSmall
                    spacing: Theme.spacingSmall
                    Image {
                        width: 76; height: 76
                        sourceSize: Qt.size(76, 76)
                        source: root.iconUrl(root.service ? root.service.iconCode : "03d")
                        Accessible.ignored: true
                    }
                    Column {
                        width: parent.width - 92
                        spacing: 1
                        Text { color: Theme.textPrimary; font.family: Theme.fixedFontFamily; font.pixelSize: 34; text: root.service ? root.temperature(root.service.temperatureCelsius) : "—" }
                        Text { width: parent.width; color: Theme.textSecondary; elide: Text.ElideRight; font.family: Theme.sansFontFamily; font.pixelSize: Theme.bodyTextSize; text: root.service && root.service.condition ? root.service.condition : qsTr("Unavailable") }
                        Text { color: Theme.textMuted; font.family: Theme.fixedFontFamily; font.pixelSize: Theme.captionTextSize; text: root.service ? qsTr("FEELS %1 · H %2 / L %3").arg(root.temperature(root.service.feelsLikeCelsius)).arg(root.temperature(root.service.highCelsius)).arg(root.temperature(root.service.lowCelsius)) : "—" }
                        Text { color: Theme.textMuted; font.family: Theme.fixedFontFamily; font.pixelSize: Theme.captionTextSize; text: root.service ? qsTr("HUM %1% · %2 %3 km/h").arg(Math.round(root.service.humidityPercent)).arg(root.service.windDirection).arg(Math.round(root.service.windSpeedKmh)) : "—" }
                    }
                }
            }

            Dashboard.Frame {
                objectName: "hourlyForecastPanel"
                Layout.preferredWidth: 500
                Layout.fillHeight: true
                backgroundColor: Theme.surface
                color: Theme.sectionDividerStrong
                lineWidth: 1
                corners: ({ rounded: Theme.radiusMedium })
                Row {
                    anchors.fill: parent
                    anchors.margins: 5
                    Repeater {
                        model: root.service ? root.service.hourlyModel : null
                        delegate: Item {
                            required property string localTime
                            required property string iconCode
                            required property real temperatureCelsius
                            required property real precipitationProbabilityPercent
                            width: 61; height: parent.height
                            Text { anchors.horizontalCenter: parent.horizontalCenter; color: Theme.textMuted; font.family: Theme.fixedFontFamily; font.pixelSize: Theme.axisTextSize; text: localTime }
                            Image { anchors.top: parent.top; anchors.topMargin: Theme.spacingMedium; anchors.horizontalCenter: parent.horizontalCenter; width: 30; height: 30; sourceSize: Qt.size(30, 30); source: root.iconUrl(iconCode); Accessible.ignored: true }
                            Text { anchors.top: parent.top; anchors.topMargin: 48; anchors.horizontalCenter: parent.horizontalCenter; color: Theme.textPrimary; font.family: Theme.fixedFontFamily; font.pixelSize: Theme.captionTextSize; text: root.temperature(temperatureCelsius) }
                            Rectangle { anchors.bottom: parent.bottom; anchors.bottomMargin: 5; anchors.horizontalCenter: parent.horizontalCenter; width: 7; height: Math.max(2, precipitationProbabilityPercent * 0.35); color: Theme.violetAccent; radius: Theme.radiusSmall; Accessible.ignored: true }
                            Rectangle { anchors.top: parent.top; anchors.topMargin: 78 - temperatureCelsius; anchors.horizontalCenter: parent.horizontalCenter; width: 5; height: 5; radius: width / 2; color: Theme.primaryAccent; Accessible.ignored: true }
                        }
                    }
                }
            }

            Dashboard.Frame {
                objectName: "dailyForecastPanel"
                Layout.fillWidth: true
                Layout.fillHeight: true
                backgroundColor: Theme.surface
                color: Theme.sectionDividerStrong
                lineWidth: 1
                corners: ({ rounded: Theme.radiusMedium })
                Column {
                    anchors.fill: parent
                    anchors.margins: 4
                    Repeater {
                        model: root.service ? root.service.dailyModel : null
                        delegate: RowLayout {
                            required property string weekday
                            required property string iconCode
                            required property real minimumCelsius
                            required property real maximumCelsius
                            required property real precipitationProbabilityPercent
                            width: parent.width; height: 36; spacing: 4
                            Text { Layout.preferredWidth: 42; color: Theme.textSecondary; font.family: Theme.sansFontFamily; font.pixelSize: Theme.sectionTitleTextSize; text: weekday.toUpperCase() }
                            Image { Layout.preferredWidth: 28; Layout.preferredHeight: 28; sourceSize: Qt.size(28, 28); source: root.iconUrl(iconCode); Accessible.ignored: true }
                            Text { Layout.preferredWidth: 40; color: Theme.violetAccent; font.family: Theme.fixedFontFamily; font.pixelSize: Theme.captionTextSize; text: qsTr("%1%").arg(Math.round(precipitationProbabilityPercent)) }
                            Text { Layout.preferredWidth: 58; color: Theme.textPrimary; font.family: Theme.fixedFontFamily; font.pixelSize: Theme.captionTextSize; text: qsTr("%1 / %2").arg(root.temperature(minimumCelsius)).arg(root.temperature(maximumCelsius)) }
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 3; color: Theme.metricRail; radius: Theme.radiusSmall; Rectangle { width: parent.width * 0.62; height: parent.height; anchors.centerIn: parent; color: Theme.primaryAccent; radius: Theme.radiusSmall } }
                        }
                    }
                }
            }
        }
    }

    Item {
        id: refreshArea
        objectName: "weatherRefreshTarget"
        anchors.fill: parent
        activeFocusOnTab: true
        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Refresh weather")
        Keys.onReturnPressed: if (root.service) root.service.refresh()
        Keys.onSpacePressed: if (root.service) root.service.refresh()
    }
}
