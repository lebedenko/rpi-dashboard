pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Shapes

ApplicationWindow {
    id: root

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
    Shortcut { sequence: "Left"; onActivated: root.selectPreviousPage() }
    Shortcut { sequence: "Right"; onActivated: root.selectNextPage() }
    Shortcut { sequence: "F5"; onActivated: root.currentPlaceholder.forceActiveFocus() }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: sidebarSurface

            color: Theme.surface
            Layout.preferredWidth: Theme.sidebarWidth
            Layout.fillHeight: true

            Shape {
                id: sidebarSeparator

                anchors.fill: parent
                Accessible.ignored: true

                ShapePath {
                    fillColor: "transparent"
                    strokeColor: Theme.passiveBorder
                    strokeWidth: 1
                    joinStyle: ShapePath.MiterJoin
                    startX: sidebarSeparator.width - Theme.sidebarChamfer
                    startY: 0
                    PathLine { x: sidebarSeparator.width; y: Theme.sidebarChamfer }
                    PathLine { x: sidebarSeparator.width; y: sidebarSeparator.height }
                }
            }

            Column {
                anchors.centerIn: parent
                spacing: Theme.spacingMedium

                SidebarButton {
                    width: Theme.touchTarget
                    height: Theme.touchTarget
                    selected: pageStack.currentIndex === 0
                    iconSource: Qt.resolvedUrl("icons/overview.svg")
                    tooltipText: qsTr("Overview")
                    onClicked: pageStack.currentIndex = 0
                }

                SidebarButton {
                    width: Theme.touchTarget
                    height: Theme.touchTarget
                    selected: pageStack.currentIndex === 1
                    iconSource: Qt.resolvedUrl("icons/systems.svg")
                    tooltipText: qsTr("Systems")
                    onClicked: pageStack.currentIndex = 1
                }

                SidebarButton {
                    width: Theme.touchTarget
                    height: Theme.touchTarget
                    selected: pageStack.currentIndex === 2
                    iconSource: Qt.resolvedUrl("icons/projects.svg")
                    tooltipText: qsTr("Projects")
                    onClicked: pageStack.currentIndex = 2
                }

                SidebarButton {
                    width: Theme.touchTarget
                    height: Theme.touchTarget
                    selected: pageStack.currentIndex === 3
                    iconSource: Qt.resolvedUrl("icons/weather.svg")
                    tooltipText: qsTr("Weather")
                    onClicked: pageStack.currentIndex = 3
                }
            }
        }

        StackLayout {
            id: pageStack

            currentIndex: 0
            Layout.fillWidth: true
            Layout.fillHeight: true

            DashboardPage { id: overviewPage; heading: qsTr("Overview") }
            DashboardPage { id: systemsPage; heading: qsTr("Systems") }
            DashboardPage { id: projectsPage; heading: qsTr("Projects") }
            DashboardPage { id: weatherPage; heading: qsTr("Weather") }
        }
    }
}
