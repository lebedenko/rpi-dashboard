pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Rpi.Dashboard as Dashboard

FocusScope {
    id: root
    property var service: null
    property date ageClock: new Date()
    readonly property alias focusTarget: refreshArea
    readonly property string stateText: !root.service ? qsTr("Weather unavailable") : root.service.state === "unconfigured" ? qsTr("Weather is not configured") : root.service.state === "locating" ? qsTr("Finding location…") : root.service.state === "loading" ? qsTr("Updating weather…") : root.service.diagnostics
    readonly property string locationText: root.service && root.service.city ? qsTr("%1, %2").arg(root.service.city).arg(root.service.country) : qsTr("Location unavailable")
    readonly property string accessibleSummary: root.service && root.service.state === "ready" ? qsTr("Weather for %1. %2, %3 degrees Celsius. Humidity %4 percent. Wind %5 kilometres per hour.").arg(root.locationText).arg(root.service.condition).arg(Math.round(root.service.temperatureCelsius)).arg(Math.round(root.service.humidityPercent)).arg(Math.round(root.service.windSpeedKmh)) : root.stateText
    objectName: "weatherPage"
    Accessible.role: Accessible.Pane
    Accessible.name: root.accessibleSummary

    function colorHex(color): string {
        return color.toString().substring(1);
    }
    function iconUrl(code): string {
        return "image://weather/%1/%2/%3/%4".arg(code || "03d").arg(root.colorHex(Theme.textPrimary)).arg(root.colorHex(Theme.attentionStatus)).arg(root.colorHex(Theme.primaryAccent));
    }
    function temperature(value): string {
        return qsTr("%1°").arg(Math.round(Number(value)));
    }
    function relativeAge(timestamp, now): string {
        if (!timestamp)
            return root.stateText;
        const minutes = Math.max(0, Math.floor((now.getTime() - timestamp.getTime()) / 60000));
        if (minutes < 1)
            return qsTr("NOW");
        if (minutes < 60)
            return qsTr("%1m AGO").arg(minutes);
        const hours = Math.floor(minutes / 60);
        if (hours < 24)
            return qsTr("%1h AGO").arg(hours);
        return qsTr("%1d AGO").arg(Math.floor(hours / 24));
    }

    Dashboard.Frame {
        objectName: "weatherPageFrame"
        anchors {
            fill: parent
            leftMargin: Theme.spacingSmall
            rightMargin: Theme.spacingSmall
            topMargin: Theme.displaySafeInset
            bottomMargin: Theme.displaySafeInset
        }
        backgroundColor: Theme.cardSurface
        color: Theme.cardFrame
        lineWidth: 1
        corners: ({
                chamfered: Theme.chamferLarge
            })
        Accessible.ignored: true
    }
    ColumnLayout {
        anchors {
            fill: parent
            leftMargin: Theme.spacingMedium
            rightMargin: Theme.spacingMedium
            topMargin: Theme.displaySafeInset + 6
            bottomMargin: Theme.displaySafeInset + 6
        }
        spacing: 6
        Item {
            objectName: "weatherHeader"
            implicitWidth: root.width - 2 * Theme.spacingMedium
            implicitHeight: 27
            Layout.fillWidth: true
            Layout.preferredWidth: root.width - 2 * Theme.spacingMedium
            Layout.minimumHeight: 27
            Layout.preferredHeight: 27
            Layout.maximumHeight: 27
            Row {
                anchors {
                    left: parent.left
                    leftMargin: Theme.spacingMedium
                    verticalCenter: parent.verticalCenter
                }
                spacing: Theme.spacingSmall
                Text {
                    id: weatherTitle
                    objectName: "weatherTitle"
                    color: Theme.primaryAccent
                    font {
                        family: Theme.sansFontFamily
                        pixelSize: Theme.headingTextSize
                        weight: Theme.headingFontWeight
                    }
                    text: qsTr("WEATHER")
                }
                Text {
                    anchors.baseline: weatherTitle.baseline
                    objectName: "weatherLocation"
                    color: Theme.textPrimary
                    elide: Text.ElideRight
                    font {
                        family: Theme.sansFontFamily
                        pixelSize: Theme.secondaryHeadingTextSize
                        weight: Theme.headingFontWeight
                    }
                    text: root.locationText.toUpperCase()
                }
            }
            Row {
                anchors {
                    right: parent.right
                    rightMargin: Theme.spacingMedium
                }
                y: weatherTitle.parent.y + weatherTitle.y + weatherTitle.baselineOffset - weatherUpdatedLabel.baselineOffset
                spacing: Theme.spacingSmall
                Rectangle {
                    objectName: "weatherStatusMarker"
                    width: 6
                    height: 6
                    anchors.verticalCenter: weatherUpdatedLabel.verticalCenter
                    radius: 3
                    color: root.service && (root.service.stale || root.service.state === "error") ? Theme.attentionStatus : Theme.primaryAccent
                }
                Text {
                    id: weatherUpdatedLabel
                    objectName: "weatherUpdatedLabel"
                    color: Theme.primaryAccent
                    font {
                        family: Theme.sansFontFamily
                        pixelSize: Theme.secondaryHeadingTextSize
                        weight: Theme.headingFontWeight
                    }
                    text: qsTr("UPDATED")
                }
                Text {
                    anchors.baseline: weatherUpdatedLabel.baseline
                    objectName: "weatherAgeLabel"
                    color: Theme.textPrimary
                    font {
                        family: Theme.sansFontFamily
                        pixelSize: Theme.secondaryHeadingTextSize
                        weight: Theme.headingFontWeight
                    }
                    text: root.relativeAge(root.service ? root.service.lastSuccessUtc : null, root.ageClock)
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingSmall
            Dashboard.Frame {
                objectName: "currentConditionsPanel"
                Layout.preferredWidth: 360
                Layout.fillHeight: true
                backgroundColor: Theme.surface
                color: Theme.sectionDividerStrong
                lineWidth: 1
                corners: ({
                        chamfered: Theme.chamferLarge
                    })
                Item {
                    id: currentContent
                    objectName: "currentContent"
                    anchors {
                        fill: parent
                        leftMargin: Theme.spacingMedium
                        rightMargin: Theme.spacingMedium
                        topMargin: Theme.spacingSmall
                        bottomMargin: Theme.spacingSmall
                    }

                    Row {
                        id: currentHero
                        objectName: "currentHero"
                        anchors.top: parent.top
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: childrenRect.width
                        height: 100
                        spacing: 20
                        Image {
                            width: 96
                            height: 96
                            sourceSize: Qt.size(96, 96)
                            source: root.iconUrl(root.service ? root.service.iconCode : "03d")
                            Accessible.ignored: true
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            color: Theme.textPrimary
                            font {
                                family: Theme.fixedFontFamily
                                pixelSize: Theme.clockTimeTextSize
                                weight: Theme.metricFontWeight
                            }
                            text: root.service ? root.temperature(root.service.temperatureCelsius) : "—"
                        }
                    }
                    Text {
                        id: currentCondition
                        objectName: "currentCondition"
                        anchors.top: currentHero.bottom
                        anchors.topMargin: 2
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        color: Theme.primaryAccent
                        elide: Text.ElideRight
                        font {
                            family: Theme.sansFontFamily
                            pixelSize: Theme.headingTextSize
                            weight: Theme.headingFontWeight
                        }
                        text: root.service && root.service.condition ? root.service.condition.toUpperCase() : qsTr("UNAVAILABLE")
                    }
                    Text {
                        objectName: "currentFeelsLike"
                        anchors.top: currentCondition.bottom
                        anchors.topMargin: 2
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        color: Theme.textPrimary
                        font {
                            family: Theme.sansFontFamily
                            pixelSize: Theme.secondaryHeadingTextSize
                        }
                        text: root.service ? qsTr("FEELS LIKE %1").arg(root.temperature(root.service.feelsLikeCelsius)) : "—"
                    }
                    Item {
                        objectName: "currentSeparatorArea"
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: currentMetrics.top
                        width: parent.width
                        height: 17
                        Separator {
                            objectName: "currentSeparator"
                            width: parent.width
                            anchors.centerIn: parent
                            orientation: Qt.Horizontal
                            lineStyle: Separator.Solid
                            color: Theme.sectionDividerStrong
                        }
                    }
                    Row {
                        id: currentMetrics
                        objectName: "currentMetrics"
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 44
                        Repeater {
                            model: [qsTr("H"), qsTr("L"), qsTr("HUMIDITY"), qsTr("WIND")]
                            delegate: Item {
                                id: metricDelegate
                                required property string modelData
                                required property int index
                                objectName: "currentMetricColumn" + metricDelegate.index
                                width: [64, 64, 85, 115][metricDelegate.index]
                                height: parent.height
                                Separator {
                                    objectName: "currentMetricSeparator" + metricDelegate.index
                                    visible: metricDelegate.index > 0
                                    anchors {
                                        left: parent.left
                                        top: parent.top
                                        bottom: parent.bottom
                                    }
                                    orientation: Qt.Vertical
                                    lineStyle: Separator.Dotted
                                    color: Theme.sectionDividerStrong
                                }
                                Item {
                                    id: metricContent
                                    objectName: "currentMetricContent" + metricDelegate.index
                                    anchors {
                                        fill: parent
                                        leftMargin: Theme.spacingSmall
                                        rightMargin: Theme.spacingSmall
                                    }
                                    Row {
                                        objectName: "currentMetricInline" + metricDelegate.index
                                        anchors.centerIn: parent
                                        spacing: 4
                                        visible: metricDelegate.index < 2

                                        Text {
                                            objectName: metricDelegate.index < 2 ? "currentMetricLabel" + metricDelegate.index : ""
                                            color: Theme.primaryAccent
                                            horizontalAlignment: Text.AlignHCenter
                                            font {
                                                family: Theme.sansFontFamily
                                                pixelSize: Theme.metricTextSize
                                            }
                                            text: metricDelegate.modelData
                                        }

                                        Text {
                                            objectName: metricDelegate.index < 2 ? "currentMetricValue" + metricDelegate.index : ""
                                            color: Theme.textPrimary
                                            horizontalAlignment: Text.AlignHCenter
                                            font {
                                                family: Theme.fixedFontFamily
                                                pixelSize: Theme.metricTextSize
                                            }
                                            text: !root.service ? "—" : metricDelegate.index === 0 ? root.temperature(root.service.highCelsius) : root.temperature(root.service.lowCelsius)
                                        }
                                    }

                                    Column {
                                        anchors.centerIn: parent
                                        width: parent.width
                                        spacing: 2
                                        visible: metricDelegate.index >= 2

                                        Text {
                                            width: parent.width
                                            objectName: metricDelegate.index >= 2 ? "currentMetricLabel" + metricDelegate.index : ""
                                            horizontalAlignment: Text.AlignHCenter
                                            color: Theme.primaryAccent
                                            font {
                                                family: Theme.sansFontFamily
                                                pixelSize: Theme.metricTextSize
                                            }
                                            text: metricDelegate.modelData
                                        }
                                        Text {
                                            width: parent.width
                                            objectName: metricDelegate.index >= 2 ? "currentMetricValue" + metricDelegate.index : ""
                                            horizontalAlignment: Text.AlignHCenter
                                            color: Theme.textPrimary
                                            fontSizeMode: metricDelegate.index === 3 ? Text.HorizontalFit : Text.FixedSize
                                            minimumPixelSize: Theme.bodyTextSize
                                            font {
                                                family: Theme.fixedFontFamily
                                                pixelSize: Theme.metricTextSize
                                            }
                                            text: !root.service ? "—" : metricDelegate.index === 2 ? qsTr("%1%").arg(Math.round(root.service.humidityPercent)) : qsTr("%1 %2 km/h").arg(root.service.windDirection).arg(Math.round(root.service.windSpeedKmh))
                                        }
                                    }
                                }
                            }
                        }
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
                corners: ({
                        chamfered: Theme.chamferLarge
                    })
                Column {
                    anchors {
                        fill: parent
                        margins: 8
                    }
                    spacing: 8
                    Text {
                        objectName: "hourlyForecastTitle"
                        x: Theme.spacingSmall
                        height: implicitHeight
                        color: Theme.primaryAccent
                        font {
                            family: Theme.sansFontFamily
                            pixelSize: Theme.secondaryHeadingTextSize
                            weight: Theme.headingFontWeight
                        }
                        text: qsTr("NEXT 8 HOURS")
                    }
                    Row {
                        width: parent.width
                        height: parent.height - parent.spacing - parent.children[0].height
                        Repeater {
                            model: root.service ? root.service.hourlyModel : null
                            delegate: Item {
                                id: hourlyDelegate
                                required property int index
                                required property string localHour
                                required property string iconCode
                                required property real temperatureCelsius
                                required property real precipitationProbabilityPercent
                                required property real trendPosition
                                required property real previousTrendPosition
                                width: parent.width / 8
                                height: parent.height
                                Separator {
                                    objectName: "hourlySeparator" + hourlyDelegate.index
                                    visible: hourlyDelegate.index > 0
                                    anchors {
                                        left: parent.left
                                        top: parent.top
                                        bottom: parent.bottom
                                    }
                                    orientation: Qt.Vertical
                                    lineStyle: Separator.Dotted
                                    color: Theme.sectionDividerStrong
                                }
                                Text {
                                    id: hourLabel
                                    objectName: "hourLabel" + hourlyDelegate.index
                                    width: parent.width
                                    horizontalAlignment: Text.AlignHCenter
                                    color: Theme.primaryAccent
                                    font {
                                        family: Theme.fixedFontFamily
                                        pixelSize: Theme.bodyTextSize
                                    }
                                    text: hourlyDelegate.index === 0 ? qsTr("NOW") : hourlyDelegate.localHour
                                }
                                Image {
                                    id: hourlyIcon
                                    anchors {
                                        top: hourLabel.bottom
                                        topMargin: 8
                                        horizontalCenter: parent.horizontalCenter
                                    }
                                    width: 34
                                    height: 34
                                    sourceSize: Qt.size(34, 34)
                                    source: root.iconUrl(hourlyDelegate.iconCode)
                                    Accessible.ignored: true
                                }
                                Text {
                                    id: hourlyTemperature
                                    anchors {
                                        top: hourlyIcon.bottom
                                        topMargin: 8
                                    }
                                    width: parent.width
                                    horizontalAlignment: Text.AlignHCenter
                                    color: Theme.textPrimary
                                    font {
                                        family: Theme.fixedFontFamily
                                        pixelSize: Theme.metricTextSize
                                    }
                                    text: root.temperature(hourlyDelegate.temperatureCelsius)
                                }
                                Item {
                                    id: hourlyGraph
                                    anchors {
                                        left: parent.left
                                        right: parent.right
                                        top: hourlyTemperature.bottom
                                        topMargin: 8
                                    }
                                    height: 42
                                    WeatherGraphSegment {
                                        anchors.fill: parent
                                        segmentObjectName: "hourlyGraphSegment" + hourlyDelegate.index
                                        shapeObjectName: "hourlyGraphShape" + hourlyDelegate.index
                                        segmentVisible: hourlyDelegate.index > 0
                                        position: hourlyDelegate.trendPosition
                                        previousPosition: hourlyDelegate.previousTrendPosition
                                    }
                                }
                                Rectangle {
                                    id: hourlyPrecipitationBar
                                    objectName: "hourlyPrecipitationBar" + hourlyDelegate.index
                                    anchors {
                                        bottom: parent.bottom
                                        horizontalCenter: parent.horizontalCenter
                                    }
                                    width: 7
                                    height: hourlyDelegate.precipitationProbabilityPercent > 0 ? Math.max(2, hourlyDelegate.precipitationProbabilityPercent * 0.25) : 0
                                    color: Theme.violetAccent
                                    radius: 2
                                }
                                Text {
                                    id: hourlyPrecipitation
                                    objectName: "hourlyPrecipitation" + hourlyDelegate.index
                                    anchors {
                                        bottom: hourlyDelegate.precipitationProbabilityPercent > 0 ? hourlyPrecipitationBar.top : parent.bottom
                                        bottomMargin: hourlyDelegate.precipitationProbabilityPercent > 0 ? 8 : 0
                                    }
                                    width: parent.width
                                    horizontalAlignment: Text.AlignHCenter
                                    color: hourlyDelegate.precipitationProbabilityPercent > 0 ? Theme.violetAccent : Theme.textPrimary
                                    font {
                                        family: Theme.fixedFontFamily
                                        pixelSize: Theme.bodyTextSize
                                    }
                                    text: qsTr("%1%").arg(Math.round(hourlyDelegate.precipitationProbabilityPercent))
                                }
                            }
                        }
                    }
                }
            }
            Dashboard.Frame {
                objectName: "dailyForecastPanel"
                Layout.fillWidth: true
                Layout.preferredWidth: 344
                Layout.fillHeight: true
                backgroundColor: Theme.surface
                color: Theme.sectionDividerStrong
                lineWidth: 1
                corners: ({
                        chamfered: Theme.chamferLarge
                    })
                Item {
                    id: dailyContent
                    anchors.fill: parent
                    readonly property real devicePixelRatio: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1
                    readonly property real weekdayGroupWidth: Math.ceil(dailyWeekdayMetrics.advanceWidth / 2) * 2
                    readonly property real precipitationGroupWidth: Math.ceil(dailyPrecipitationMetrics.advanceWidth / 2) * 2

                    TextMetrics {
                        id: dailyWeekdayMetrics
                        objectName: "dailyWeekdayMetrics"
                        font {
                            family: Theme.sansFontFamily
                            pixelSize: Theme.sectionTitleTextSize
                        }
                        text: qsTr("TODAY")
                    }

                    TextMetrics {
                        id: dailyPrecipitationMetrics
                        objectName: "dailyPrecipitationMetrics"
                        font {
                            family: Theme.fixedFontFamily
                            pixelSize: Theme.captionTextSize
                        }
                        text: qsTr("%1%").arg(100)
                    }

                    function boundary(rowIndex: int): real {
                        const titleHeight = 32;
                        const physicalPosition = (titleHeight + rowIndex * (dailyContent.height - titleHeight) / 5) * dailyContent.devicePixelRatio;
                        return Math.round(physicalPosition) / dailyContent.devicePixelRatio;
                    }

                    Text {
                        objectName: "dailyForecastTitle"
                        x: Theme.spacingMedium
                        y: Theme.spacingSmall
                        height: 24
                        color: Theme.primaryAccent
                        font {
                            family: Theme.sansFontFamily
                            pixelSize: Theme.secondaryHeadingTextSize
                            weight: Theme.headingFontWeight
                        }
                        text: qsTr("5 DAYS FORECAST")
                    }
                    Repeater {
                        model: root.service ? root.service.dailyModel : null
                        delegate: Item {
                            id: dailyDelegate
                            required property int index
                            required property string weekday
                            required property string iconCode
                            required property real minimumCelsius
                            required property real maximumCelsius
                            required property var averageCelsius
                            required property real precipitationProbabilityPercent
                            x: 0
                            y: dailyContent.boundary(dailyDelegate.index)
                            width: dailyContent.width
                            height: dailyContent.boundary(dailyDelegate.index + 1) - y
                            Separator {
                                objectName: "dailySeparator" + dailyDelegate.index
                                visible: dailyDelegate.index > 0
                                anchors {
                                    left: parent.left
                                    right: parent.right
                                    top: parent.top
                                    leftMargin: Theme.spacingMedium
                                    rightMargin: Theme.spacingMedium
                                }
                                orientation: Qt.Horizontal
                                lineStyle: Separator.Solid
                                color: Theme.sectionDividerStrong
                            }
                            Row {
                                objectName: "dailyRow" + dailyDelegate.index
                                anchors {
                                    fill: parent
                                    leftMargin: Theme.spacingMedium
                                    rightMargin: Theme.spacingMedium
                                }
                                spacing: Theme.spacingMedium
                                Item {
                                    objectName: "dailyWeekdayGroup" + dailyDelegate.index
                                    width: dailyContent.weekdayGroupWidth
                                    height: parent.height
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        objectName: "dailyWeekday" + dailyDelegate.index
                                        color: Theme.primaryAccent
                                        font {
                                            family: Theme.sansFontFamily
                                            pixelSize: Theme.sectionTitleTextSize
                                        }
                                        text: dailyDelegate.index === 0 ? qsTr("TODAY") : dailyDelegate.weekday.toUpperCase()
                                    }
                                }
                                Item {
                                    objectName: "dailyIconGroup" + dailyDelegate.index
                                    width: 28
                                    height: parent.height
                                    Image {
                                        objectName: "dailyIcon" + dailyDelegate.index
                                        anchors.centerIn: parent
                                        width: 28
                                        height: 28
                                        sourceSize: Qt.size(28, 28)
                                        source: root.iconUrl(dailyDelegate.iconCode)
                                        Accessible.ignored: true
                                    }
                                }
                                Item {
                                    objectName: "dailyPrecipitationGroup" + dailyDelegate.index
                                    width: dailyContent.precipitationGroupWidth
                                    height: parent.height
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        objectName: "dailyPrecipitation" + dailyDelegate.index
                                        horizontalAlignment: Text.AlignLeft
                                        color: Theme.violetAccent
                                        font {
                                            family: Theme.fixedFontFamily
                                            pixelSize: Theme.captionTextSize
                                        }
                                        text: qsTr("%1%").arg(Math.round(dailyDelegate.precipitationProbabilityPercent))
                                    }
                                }
                                Row {
                                    objectName: "dailyTemperatureGroup" + dailyDelegate.index
                                    width: parent.width - dailyContent.weekdayGroupWidth - 28 - dailyContent.precipitationGroupWidth - 3 * parent.spacing
                                    height: parent.height
                                    spacing: 8
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 30
                                        objectName: "dailyMinimum" + dailyDelegate.index
                                        horizontalAlignment: Text.AlignRight
                                        color: Theme.primaryAccent
                                        font {
                                            family: Theme.fixedFontFamily
                                            pixelSize: Theme.captionTextSize
                                        }
                                        text: root.temperature(dailyDelegate.minimumCelsius)
                                    }
                                    TemperatureSegment {
                                        objectName: "dailyTemperatureSegment" + dailyDelegate.index
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: parent.width - 30 - dailyMaximum.width - 2 * parent.spacing
                                        height: 10
                                        knobVisible: dailyDelegate.averageCelsius !== undefined && dailyDelegate.averageCelsius !== null
                                        position: dailyDelegate.maximumCelsius === dailyDelegate.minimumCelsius ? 0.5 : (Number(dailyDelegate.averageCelsius) - dailyDelegate.minimumCelsius) / (dailyDelegate.maximumCelsius - dailyDelegate.minimumCelsius)
                                    }
                                    Text {
                                        id: dailyMaximum
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: implicitWidth
                                        objectName: "dailyMaximum" + dailyDelegate.index
                                        horizontalAlignment: Text.AlignLeft
                                        color: Theme.attentionStatus
                                        font {
                                            family: Theme.fixedFontFamily
                                            pixelSize: Theme.captionTextSize
                                        }
                                        text: root.temperature(dailyDelegate.maximumCelsius)
                                    }
                                }
                            }
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
        Keys.onReturnPressed: if (root.service)
            root.service.refresh()
        Keys.onSpacePressed: if (root.service)
            root.service.refresh()
    }
    Timer {
        interval: 60000
        repeat: true
        running: root.visible
        onTriggered: root.ageClock = new Date()
    }
}
