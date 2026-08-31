pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: root

    property var service: null
    readonly property string iconCode: root.validIconCode(root.service ? root.service.iconCode : "")
    readonly property url wallpaperSource: "qrc:/rpi-dashboard/weather-bg/" + root.iconCode + ".png"
    readonly property bool weatherReady: root.service && root.service.state === "ready"
    readonly property bool dayScene: root.iconCode.endsWith("d")
    readonly property string locationText: root.service && root.service.city
                                           ? qsTr("%1, %2").arg(root.service.city).arg(root.service.country)
                                           : qsTr("Location unavailable")

    objectName: "screensaverView"
    Accessible.role: Accessible.Pane
    Accessible.name: root.weatherReady
                     ? qsTr("%1, %2, %3 degrees Celsius").arg(root.locationText)
                         .arg(root.service.condition).arg(Math.round(root.service.temperatureCelsius))
                     : qsTr("Weather unavailable")

    function validIconCode(code): string {
        const supported = ["01d", "01n", "02d", "02n", "03d", "03n", "04d", "04n", "09d",
                           "09n", "10d", "10n", "11d", "11n", "13d", "13n", "50d", "50n"]
        return supported.includes(String(code)) ? String(code) : "03d"
    }

    function temperature(value): string {
        return Number.isFinite(Number(value)) ? qsTr("%1°").arg(Math.round(Number(value))) : "—"
    }

    Image {
        id: wallpaper
        objectName: "screensaverWallpaper"
        anchors.fill: parent
        source: root.wallpaperSource
        sourceSize: Qt.size(1480, 320)
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        Accessible.ignored: true
    }

    Rectangle {
        objectName: "screensaverLeftScrim"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: Math.min(1120, parent.width)
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop {
                objectName: "screensaverScrimStart"
                position: 0.0
                color: Qt.rgba(Theme.background.r, Theme.background.g, Theme.background.b,
                               root.dayScene ? 0.68 : 0.24)
            }
            GradientStop {
                objectName: "screensaverScrimMiddle"
                position: 0.85
                color: Qt.rgba(Theme.background.r, Theme.background.g, Theme.background.b,
                               root.dayScene ? 0.58 : 0.18)
            }
            GradientStop {
                objectName: "screensaverScrimEnd"
                position: 1.0
                color: Qt.rgba(Theme.background.r, Theme.background.g, Theme.background.b, 0.0)
            }
        }
        Accessible.ignored: true
    }

    Item {
        id: contentRow
        anchors.left: parent.left
        anchors.leftMargin: 64
        anchors.verticalCenter: parent.verticalCenter
        implicitWidth: conditionBlock.x + conditionBlock.width
        implicitHeight: temperatureText.height

        Image {
            objectName: "screensaverConditionIcon"
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 110
            height: 110
            sourceSize: Qt.size(110, 110)
            fillMode: Image.PreserveAspectFit
            source: "image://weather/" + root.iconCode + "/5de7ff/ffc857/a66cff"
            Accessible.ignored: true
        }

        Text {
            id: temperatureText
            objectName: "screensaverTemperature"
            anchors.left: parent.left
            anchors.leftMargin: 142
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.focusAccent
            font.family: Theme.sansFontFamily
            font.pixelSize: 152
            font.weight: Font.Light
            text: root.weatherReady ? root.temperature(root.service.temperatureCelsius) : "—"
        }

        Item {
            id: conditionBlock
            objectName: "screensaverConditionBlock"
            anchors.left: temperatureText.right
            anchors.leftMargin: 32
            anchors.top: temperatureText.top
            anchors.bottom: temperatureText.bottom
            implicitWidth: Math.max(conditionText.width, locationRow.implicitWidth)
            width: implicitWidth

            Text {
                id: conditionText
                objectName: "screensaverCondition"
                y: locationRow.y - height + 6
                color: root.weatherReady ? Theme.focusAccent : Theme.textSecondary
                font.family: Theme.sansFontFamily
                font.pixelSize: 32
                font.weight: Font.Normal
                text: root.weatherReady && root.service.condition ? root.service.condition.toUpperCase()
                                                                        : qsTr("WEATHER UNAVAILABLE")
            }

            Item {
                id: locationRow
                objectName: "screensaverLocationRow"
                y: temperatureText.baselineOffset - locationText.baselineOffset
                implicitWidth: locationText.width + (statusText.visible ? statusText.width + 10 : 0)
                implicitHeight: locationText.height
                width: implicitWidth
                height: implicitHeight

                Text {
                    id: locationText
                    objectName: "screensaverLocation"
                    color: Theme.textMuted
                    font.family: Theme.sansFontFamily
                    font.pixelSize: 24
                    font.weight: Font.Light
                    text: root.locationText.toUpperCase()
                }

                Text {
                    id: statusText
                    objectName: "screensaverStatus"
                    x: locationText.width + 10
                    y: locationText.baselineOffset - baselineOffset
                    visible: root.service && root.service.stale
                    color: Theme.staleStatus
                    font.family: Theme.fixedFontFamily
                    font.pixelSize: 12
                    text: qsTr("STALE")
                }
            }
        }
    }

    Rectangle {
        objectName: "screensaverDetailsBacking"
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        implicitWidth: detailsRow.implicitWidth + 24
        implicitHeight: detailsRow.implicitHeight + 12
        radius: Theme.radiusMedium
        border.width: 0
        color: Qt.rgba(Theme.background.r, Theme.background.g, Theme.background.b,
                       root.dayScene ? 0.78 : 0.62)

        Row {
            id: detailsRow
            objectName: "screensaverDetailsRow"
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 6
            spacing: 8

            Text { objectName: "screensaverFeelsLabel"; color: Theme.textMuted; font.family: Theme.sansFontFamily; font.pixelSize: 22; text: qsTr("FEELS") }
            Text { objectName: "screensaverFeelsValue"; color: Theme.focusAccent; font.family: Theme.sansFontFamily; font.pixelSize: 22; text: root.weatherReady ? root.temperature(root.service.feelsLikeCelsius) : "—" }
            Text { objectName: "screensaverFirstSeparator"; color: Theme.textMuted; font.family: Theme.sansFontFamily; font.pixelSize: 22; text: "|" }
            Text { objectName: "screensaverHighLabel"; color: Theme.textMuted; font.family: Theme.sansFontFamily; font.pixelSize: 22; text: qsTr("H") }
            Text { objectName: "screensaverHighValue"; color: Theme.focusAccent; font.family: Theme.sansFontFamily; font.pixelSize: 22; text: root.weatherReady ? root.temperature(root.service.highCelsius) : "—" }
            Text { objectName: "screensaverRangeSeparator"; color: Theme.violetAccent; font.family: Theme.sansFontFamily; font.pixelSize: 22; text: "·" }
            Text { objectName: "screensaverLowLabel"; color: Theme.textMuted; font.family: Theme.sansFontFamily; font.pixelSize: 22; text: qsTr("L") }
            Text { objectName: "screensaverLowValue"; color: Theme.focusAccent; font.family: Theme.sansFontFamily; font.pixelSize: 22; text: root.weatherReady ? root.temperature(root.service.lowCelsius) : "—" }
            Text { objectName: "screensaverSecondSeparator"; color: Theme.textMuted; font.family: Theme.sansFontFamily; font.pixelSize: 22; text: "|" }
            Text {
                objectName: "screensaverSolarEventLabel"
                color: Theme.textMuted
                font.family: Theme.sansFontFamily
                font.pixelSize: 22
                text: root.service && root.service.nextSolarEventKind === "sunrise" ? qsTr("SUNRISE") : qsTr("SUNSET")
            }
            Text {
                objectName: "screensaverSolarEventTime"
                color: Theme.attentionStatus
                font.family: Theme.sansFontFamily
                font.pixelSize: 22
                text: root.weatherReady && root.service.localNextSolarEventTime
                      ? root.service.localNextSolarEventTime : "—"
            }
        }
    }
}
