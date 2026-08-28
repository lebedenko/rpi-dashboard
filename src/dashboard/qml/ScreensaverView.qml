pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: root

    property var service: null
    readonly property string iconCode: root.validIconCode(root.service ? root.service.iconCode : "")
    readonly property url wallpaperSource: "qrc:/rpi-dashboard/weather-bg/" + root.iconCode + ".png"
    readonly property bool weatherReady: root.service && root.service.state === "ready"
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

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 84
        anchors.verticalCenter: parent.verticalCenter
        spacing: 38

        Image {
            objectName: "screensaverConditionIcon"
            width: 220
            height: 150
            sourceSize: Qt.size(220, 150)
            fillMode: Image.PreserveAspectFit
            source: "image://weather/" + root.iconCode + "/5de7ff/ffc857/a66cff"
            Accessible.ignored: true
        }

        Text {
            objectName: "screensaverTemperature"
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.focusAccent
            font.family: Theme.sansFontFamily
            font.pixelSize: 164
            font.weight: Font.Light
            text: root.weatherReady ? root.temperature(root.service.temperatureCelsius) : "—"
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 34
            spacing: 4

            Text {
                objectName: "screensaverCondition"
                color: root.weatherReady ? Theme.focusAccent : Theme.textSecondary
                font.family: Theme.sansFontFamily
                font.pixelSize: 32
                font.weight: Font.Medium
                text: root.weatherReady && root.service.condition ? root.service.condition.toUpperCase()
                                                                        : qsTr("WEATHER UNAVAILABLE")
            }
            Text {
                objectName: "screensaverLocation"
                color: Theme.textMuted
                font.family: Theme.sansFontFamily
                font.pixelSize: 25
                font.weight: Font.Light
                text: root.locationText.toUpperCase()
            }
            Text {
                objectName: "screensaverStatus"
                visible: root.service && root.service.stale
                color: Theme.staleStatus
                font.family: Theme.fixedFontFamily
                font.pixelSize: 12
                text: qsTr("STALE")
            }
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 62
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 28
        spacing: 26

        Text {
            objectName: "screensaverFeelsLike"
            color: Theme.textSecondary
            font.family: Theme.sansFontFamily
            font.pixelSize: 22
            text: qsTr("FEELS %1").arg(root.weatherReady ? root.temperature(root.service.feelsLikeCelsius) : "—")
        }
        Text { color: Theme.textMuted; font.pixelSize: 22; text: "|" }
        Text {
            objectName: "screensaverRange"
            color: Theme.textSecondary
            font.family: Theme.sansFontFamily
            font.pixelSize: 22
            text: qsTr("H %1  ·  L %2").arg(root.weatherReady ? root.temperature(root.service.highCelsius) : "—")
                                      .arg(root.weatherReady ? root.temperature(root.service.lowCelsius) : "—")
        }
        Text { color: Theme.textMuted; font.pixelSize: 22; text: "|" }
        Text {
            objectName: "screensaverSunset"
            color: Theme.attentionStatus
            font.family: Theme.sansFontFamily
            font.pixelSize: 22
            text: qsTr("SUNSET %1").arg(root.weatherReady && root.service.localSunset
                                        ? root.service.localSunset : "—")
        }
    }
}
