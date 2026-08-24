pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: window

    readonly property var currentPlaceholder: pageStack.currentIndex === 0 ? overviewPage.placeholder
                                               : pageStack.currentIndex === 1 ? systemsPage.placeholder
                                               : pageStack.currentIndex === 2 ? projectsPage.placeholder
                                               : weatherPage.placeholder

    width: 1480
    height: 320
    visibility: Window.FullScreen
    color: Theme.background
    title: qsTr("HoloNight Dashboard")

    function selectPreviousPage(): void {
        pageStack.currentIndex = Math.max(0, pageStack.currentIndex - 1)
    }

    function selectNextPage(): void {
        pageStack.currentIndex = Math.min(pageStack.count - 1, pageStack.currentIndex + 1)
    }

    Shortcut { sequence: "Home"; onActivated: pageStack.currentIndex = 0 }
    Shortcut { sequence: "Left"; onActivated: window.selectPreviousPage() }
    Shortcut { sequence: "Right"; onActivated: window.selectNextPage() }
    Shortcut { sequence: "F5"; onActivated: window.currentPlaceholder.forceActiveFocus() }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: sidebarSurface

            Layout.preferredWidth: 184
            Layout.fillHeight: true
            color: Theme.surface

            Rectangle {
                id: sidebarSeparator

                anchors.top: parent.top
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                width: 1
                color: Theme.passiveBorder
                Accessible.ignored: true
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: Math.max(Theme.spacingMedium, Theme.displaySafeInset)
                anchors.topMargin: Math.max(Theme.spacingMedium, Theme.displaySafeInset)
                anchors.rightMargin: Theme.spacingMedium
                anchors.bottomMargin: Math.max(Theme.spacingMedium, Theme.displaySafeInset)
                spacing: Theme.spacingSmall

                Label {
                    Layout.fillWidth: true
                    Layout.bottomMargin: Theme.spacingSmall
                    text: qsTr("Dashboard")
                    color: Theme.textPrimary
                    font.family: Theme.sansFontFamily
                    font.pixelSize: 21
                    font.weight: Theme.headingFontWeight
                }

                Repeater {
                    model: [
                        { "mark": "O", "label": qsTr("Overview") },
                        { "mark": "S", "label": qsTr("Systems") },
                        { "mark": "P", "label": qsTr("Projects") },
                        { "mark": "W", "label": qsTr("Weather") }
                    ]

                    delegate: Button {
                        id: navigationButton

                        required property var modelData
                        required property int index
                        readonly property bool selected: pageStack.currentIndex === navigationButton.index

                        Layout.fillWidth: true
                        Layout.preferredHeight: Theme.touchTarget
                        activeFocusOnTab: true
                        Accessible.name: navigationButton.modelData.label
                        onClicked: pageStack.currentIndex = navigationButton.index

                        contentItem: RowLayout {
                            spacing: Theme.spacingSmall

                            Label {
                                Layout.preferredWidth: 32
                                text: navigationButton.modelData.mark
                                color: navigationButton.selected ? Theme.primaryAccent : Theme.textMuted
                                horizontalAlignment: Text.AlignHCenter
                                font.family: Theme.sansFontFamily
                                font.pixelSize: 18
                                font.weight: Theme.headingFontWeight
                                Accessible.ignored: true
                            }

                            Label {
                                Layout.fillWidth: true
                                text: navigationButton.modelData.label
                                color: navigationButton.selected ? Theme.textPrimary : Theme.textSecondary
                                font.family: Theme.sansFontFamily
                                font.pixelSize: 16
                                font.weight: Theme.headingFontWeight
                                Accessible.ignored: true
                            }
                        }

                        background: Rectangle {
                            color: navigationButton.selected ? Theme.surfaceRaised : "transparent"
                            border.width: navigationButton.activeFocus ? 2 : 0
                            border.color: Theme.focusAccent
                            radius: 6

                            Rectangle {
                                width: 4
                                height: parent.height
                                color: navigationButton.selected ? Theme.primaryAccent : "transparent"
                                radius: 2
                                Accessible.ignored: true
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        StackLayout {
            id: pageStack

            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: 0

            DashboardPage { id: overviewPage; heading: qsTr("Overview") }
            DashboardPage { id: systemsPage; heading: qsTr("Systems") }
            DashboardPage { id: projectsPage; heading: qsTr("Projects") }
            DashboardPage { id: weatherPage; heading: qsTr("Weather") }
        }
    }
}
