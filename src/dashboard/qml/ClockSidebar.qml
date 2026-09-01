import QtQuick

Item {
    id: root

    property date currentTimestamp: new Date()
    property var projectsService: null
    property var weatherService: null
    property string pageContext: "overview"
    readonly property string timeText: Qt.formatTime(root.currentTimestamp, "hh:mm")
    readonly property string dateText: Qt.formatDate(root.currentTimestamp, "ddd dd MMM")
    readonly property string ciText: qsTr("%1 CI").arg(root.statusLabel(root.projectsService ? root.projectsService.aggregateHealth : "unknown"))
    readonly property string runnerText: root.projectsService && root.projectsService.totalRunnerCount >= 0 ? qsTr("%1/%2 RUNNERS").arg(root.projectsService.onlineRunnerCount).arg(root.projectsService.totalRunnerCount) : qsTr("— RUNNERS")
    readonly property real precipitationProbability: root.weatherService ? root.weatherService.todayPrecipitationProbabilityPercent : 0
    readonly property string precipitationLabel: root.precipitationProbability > 0 && root.weatherService ? root.precipitationName(root.weatherService.todayPrecipitationKind) : qsTr("PRECIPITATION")
    readonly property string precipitationValue: root.precipitationProbability > 0 ? qsTr("%1%").arg(Math.round(root.precipitationProbability)) : qsTr("NONE")
    readonly property string accessibleText: root.pageContext === "projects" ? qsTr("%1, %2, %3, %4").arg(root.timeText).arg(root.dateText).arg(root.ciText).arg(root.runnerText) : root.pageContext === "weather" ? qsTr("%1, %2, air %3, index %4, %5 %6, %7 %8").arg(root.timeText).arg(root.dateText).arg(root.weatherService ? root.weatherService.airQualityCategory : qsTr("unavailable")).arg(root.weatherService ? root.weatherService.airQualityIndex : 0).arg(root.weatherService && root.weatherService.nextSolarEventKind === "sunrise" ? qsTr("sunrise") : qsTr("sunset")).arg(root.weatherService ? root.weatherService.localNextSolarEventTime : "—").arg(root.precipitationLabel).arg(root.precipitationValue) : qsTr("%1, %2").arg(root.timeText).arg(root.dateText)

    function statusLabel(health) : string {
        const labels = {
            "failed": qsTr("FAILED"),
            "attention": qsTr("ATTENTION"),
            "running": qsTr("RUNNING"),
            "stale": qsTr("STALE"),
            "healthy": qsTr("HEALTHY"),
            "unknown": qsTr("UNKNOWN")
        };
        return labels[health] || labels.unknown;
    }

    function precipitationName(kind) : string {
        const labels = {
            "rain": qsTr("RAIN"),
            "snow": qsTr("SNOW"),
            "mixed": qsTr("MIXED"),
            "other": qsTr("PRECIPITATION"),
            "none": qsTr("PRECIPITATION")
        };
        return labels[kind] || labels.other;
    }

    function airQualityColor(index) : color {
        if (index >= 1 && index <= 2)
            return Theme.onlineStatus;

        if (index === 3)
            return Theme.attentionStatus;

        if (index >= 4 && index <= 5)
            return Theme.failureStatus;

        return Theme.textPrimary;
    }

    objectName: "clockSidebar"
    Accessible.role: Accessible.StaticText
    Accessible.name: root.accessibleText

    Frame {
        anchors.fill: parent
        backgroundColor: Theme.surface
        color: Theme.passiveBorder
        corners: ({
            "topLeft": {
                "rounded": Theme.radiusMedium
            },
            "topRight": {
                "chamfered": Theme.chamferLarge
            },
            "bottomRight": {
                "chamfered": Theme.chamferLarge
            },
            "bottomLeft": {
                "chamfered": Theme.chamferLarge
            }
        })
    }

    Column {
        spacing: 5

        anchors {
            top: parent.top
            topMargin: Theme.spacingSmall
            left: parent.left
            right: parent.right
        }

        Text {
            id: timeLabel

            objectName: "clockTimeLabel"
            width: parent.width
            color: Theme.primaryAccent
            horizontalAlignment: Text.AlignHCenter
            text: root.timeText
            Accessible.ignored: true

            font {
                family: Theme.sansFontFamily
                pixelSize: Theme.clockTimeTextSize
                weight: Theme.metricFontWeight
            }

        }

        Text {
            id: dateLabel

            objectName: "clockDateLabel"
            width: parent.width
            color: Theme.violetAccent
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            text: root.dateText
            Accessible.ignored: true

            font {
                family: Theme.sansFontFamily
                pixelSize: Theme.clockDateTextSize
                weight: Theme.informationFontWeight
            }

        }

        Separator {
            objectName: "clockDateSeparator"
            width: parent.width - Theme.spacingMedium
            anchors.horizontalCenter: parent.horizontalCenter
            orientation: Qt.Horizontal
            lineStyle: Separator.Dotted
            color: Theme.sectionDividerStrong
        }

        Column {
            width: parent.width
            spacing: Theme.spacingSmall
            visible: root.pageContext === "projects"

            Text {
                objectName: "globalCiHealth"
                width: parent.width
                color: Theme.statusColor(root.projectsService ? root.projectsService.aggregateHealth : "unknown")
                horizontalAlignment: Text.AlignHCenter
                text: root.ciText
                Accessible.ignored: true

                font {
                    family: Theme.sansFontFamily
                    pixelSize: Theme.sectionTitleTextSize
                    weight: Theme.technicalFontWeight
                }

            }

            Text {
                objectName: "globalRunnerHealth"
                width: parent.width
                color: Theme.textSecondary
                horizontalAlignment: Text.AlignHCenter
                text: root.runnerText
                Accessible.ignored: true

                font {
                    family: Theme.fixedFontFamily
                    pixelSize: Theme.captionTextSize
                }

            }

        }

        Column {
            objectName: "weatherRail"
            width: parent.width - 2 * Theme.spacingMedium
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Theme.spacingSmall
            visible: root.pageContext === "weather"

            Column {
                objectName: "weatherAirSection"
                width: parent.width
                spacing: 2

                Text {
                    width: parent.width
                    color: Theme.primaryAccent
                    text: qsTr("AIR")

                    font {
                        family: Theme.sansFontFamily
                        pixelSize: Theme.bodyTextSize
                    }

                }

                Row {
                    objectName: "weatherAqi"
                    spacing: Theme.spacingSmall

                    Text {
                        objectName: "weatherAqiCategory"
                        color: root.airQualityColor(root.weatherService ? root.weatherService.airQualityIndex : 0)
                        text: root.weatherService && root.weatherService.airQualityIndex ? root.weatherService.airQualityCategory.toUpperCase() : "—"

                        font {
                            family: Theme.sansFontFamily
                            pixelSize: Theme.bodyTextSize
                        }

                    }

                    Text {
                        objectName: "weatherAqiIndex"
                        color: Theme.primaryAccent
                        text: root.weatherService && root.weatherService.airQualityIndex ? String(root.weatherService.airQualityIndex) : "—"

                        font {
                            family: Theme.sansFontFamily
                            pixelSize: Theme.bodyTextSize
                        }

                    }

                }

            }

            Separator {
                objectName: "weatherRailSeparator1"
                width: parent.width
                anchors.horizontalCenter: parent.horizontalCenter
                orientation: Qt.Horizontal
                lineStyle: Separator.Dotted
                color: Theme.sectionDividerStrong
            }

            Column {
                objectName: "weatherSolarSection"
                width: parent.width
                spacing: 2

                Text {
                    width: parent.width
                    color: Theme.primaryAccent
                    text: root.weatherService && root.weatherService.nextSolarEventKind === "sunrise" ? qsTr("SUNRISE") : qsTr("SUNSET")

                    font {
                        family: Theme.sansFontFamily
                        pixelSize: Theme.bodyTextSize
                    }

                }

                Text {
                    objectName: "weatherSunset"
                    width: parent.width
                    color: Theme.textPrimary
                    text: root.weatherService && root.weatherService.localNextSolarEventTime ? root.weatherService.localNextSolarEventTime : "—"

                    font {
                        family: Theme.sansFontFamily
                        pixelSize: Theme.bodyTextSize
                    }

                }

            }

            Separator {
                objectName: "weatherRailSeparator2"
                width: parent.width
                anchors.horizontalCenter: parent.horizontalCenter
                orientation: Qt.Horizontal
                lineStyle: Separator.Dotted
                color: Theme.sectionDividerStrong
            }

            Column {
                objectName: "weatherPrecipitationSection"
                width: parent.width
                spacing: 2

                Text {
                    objectName: "weatherPrecipitationLabel"
                    width: parent.width
                    color: Theme.primaryAccent
                    text: root.precipitationLabel

                    font {
                        family: Theme.sansFontFamily
                        pixelSize: Theme.bodyTextSize
                    }

                }

                Text {
                    objectName: "weatherRain"
                    width: parent.width
                    color: Theme.textPrimary
                    text: root.precipitationValue

                    font {
                        family: Theme.sansFontFamily
                        pixelSize: Theme.bodyTextSize
                    }

                }

            }

        }

    }

    Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: root.currentTimestamp = new Date()
    }

}
