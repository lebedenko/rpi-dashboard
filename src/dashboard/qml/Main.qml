pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: window

    width: 1480
    height: 320
    minimumWidth: 960
    minimumHeight: 240
    visible: true
    color: Theme.background
    title: qsTr("HoloNight Dashboard")

    Shortcut { sequence: "Home"; onActivated: pageTabs.currentIndex = 0 }
    Shortcut { sequence: "Left"; onActivated: pageTabs.decrementCurrentIndex() }
    Shortcut { sequence: "Right"; onActivated: pageTabs.incrementCurrentIndex() }
    Shortcut { sequence: "F5"; onActivated: refreshFeedback.restart() }

    Timer {
        id: refreshFeedback
        interval: 1000
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingMedium
        spacing: Theme.spacingMedium

        Rectangle {
            Layout.preferredWidth: 72
            Layout.fillHeight: true
            color: Theme.surface
            border.color: Theme.outline

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingSmall
                spacing: Theme.spacingSmall

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "⌁"
                    color: Theme.interactive
                    font.pixelSize: 32
                    Accessible.ignored: true
                }

                Repeater {
                    model: [qsTr("Overview"), qsTr("Systems"), qsTr("Projects"), qsTr("Weather")]

                    delegate: Button {
                        id: navigationButton

                        required property string modelData
                        required property int index

                        Layout.fillWidth: true
                        Layout.preferredHeight: Theme.touchTarget
                        text: modelData.slice(0, 1)
                        Accessible.name: modelData
                        flat: true
                        onClicked: pageTabs.currentIndex = navigationButton.index

                        contentItem: Text {
                            text: navigationButton.text
                            color: pageTabs.currentIndex === navigationButton.index
                                   ? Theme.interactive : Theme.textSecondary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 20
                        }

                        background: Rectangle {
                            color: pageTabs.currentIndex === navigationButton.index
                                   ? Theme.surfaceRaised : "transparent"
                            border.color: pageTabs.currentIndex === navigationButton.index
                                          ? Theme.interactive : "transparent"
                            radius: 3
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingMedium

            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: pageTabs.currentItem ? pageTabs.currentItem.objectName : ""
                    color: Theme.textPrimary
                    font.pixelSize: 24
                    font.weight: Font.DemiBold
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: refreshFeedback.running ? qsTr("Refreshing…") : qsTr("● Online")
                    color: refreshFeedback.running ? Theme.interactive : Theme.healthy
                    font.pixelSize: 16
                }
            }

            SwipeView {
                id: pageTabs

                Layout.fillWidth: true
                Layout.fillHeight: true
                interactive: true

                RowLayout {
                    objectName: qsTr("Overview")
                    spacing: Theme.spacingMedium

                    MetricCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        label: qsTr("CPU")
                        value: qsTr("34%")
                        detail: qsTr("58°C")
                    }
                    MetricCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        label: qsTr("Memory")
                        value: qsTr("62%")
                        detail: qsTr("4.1 / 8.0 GB")
                        accent: Theme.secondary
                    }
                    MetricCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        label: qsTr("Systems")
                        value: qsTr("3 online")
                        detail: qsTr("All nominal")
                        accent: Theme.healthy
                    }
                    MetricCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        label: qsTr("Build")
                        value: qsTr("Passed")
                        detail: qsTr("project-alpha")
                    }
                    MetricCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        label: qsTr("Weather")
                        value: qsTr("18°C")
                        detail: qsTr("Lviv · Partly cloudy")
                        accent: Theme.secondary
                    }
                }

                Item { objectName: qsTr("Systems") }
                Item { objectName: qsTr("Projects") }
                Item { objectName: qsTr("Weather") }
            }
        }
    }
}
