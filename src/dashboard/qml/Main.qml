pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Shapes

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

            Layout.preferredWidth: Theme.sidebarWidth
            Layout.fillHeight: true
            color: Theme.surface

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

                Repeater {
                    model: [
                        { "icon": Qt.resolvedUrl("icons/overview.svg"), "label": qsTr("Overview") },
                        { "icon": Qt.resolvedUrl("icons/systems.svg"), "label": qsTr("Systems") },
                        { "icon": Qt.resolvedUrl("icons/projects.svg"), "label": qsTr("Projects") },
                        { "icon": Qt.resolvedUrl("icons/weather.svg"), "label": qsTr("Weather") }
                    ]

                    delegate: Loader {
                        id: navigationLoader

                        required property var modelData
                        required property int index

                        width: Theme.touchTarget
                        height: Theme.touchTarget
                        source: "_SidebarButton.qml"

                        Binding {
                            target: navigationLoader.item
                            property: "selected"
                            value: pageStack.currentIndex === navigationLoader.index
                            when: navigationLoader.status === Loader.Ready
                        }

                        Binding {
                            target: navigationLoader.item
                            property: "tooltipText"
                            value: navigationLoader.modelData.label
                            when: navigationLoader.status === Loader.Ready
                        }

                        Binding {
                            target: navigationLoader.item
                            property: "iconSource"
                            value: navigationLoader.modelData.icon
                            when: navigationLoader.status === Loader.Ready
                        }

                        Connections {
                            target: navigationLoader.status === Loader.Ready ? navigationLoader.item : null

                            function onClicked(): void {
                                pageStack.currentIndex = navigationLoader.index
                            }
                        }
                    }
                }
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
