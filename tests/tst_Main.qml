import QtQuick
import QtTest
import HoloNight.Dashboard

Item {
    id: root

    width: 1480
    height: 320

    TestCase {
        name: "DashboardWindow"
        when: windowShown

        property var dashboardWindow: null
        property var dashboardComponent: null

        function init() {
            dashboardComponent = Qt.createComponent("qrc:/qt/qml/HoloNight/Dashboard/qml/Main.qml")
            verify(dashboardComponent.status === Component.Ready, dashboardComponent.errorString())
            dashboardWindow = dashboardComponent.createObject(null, {
                "windowed": true,
                "windowWidth": 1480,
                "windowHeight": 320
            })
            verify(dashboardWindow !== null)
            dashboardWindow.show()
            tryCompare(dashboardWindow, "visible", true)
            dashboardWindow.requestActivate()
            tryCompare(dashboardWindow, "active", true)
        }

        function cleanup() {
            dashboardWindow.close()
            dashboardWindow.destroy()
            dashboardWindow = null
            dashboardComponent.destroy()
            dashboardComponent = null
        }

        function test_exactDevelopmentGeometry() {
            compare(dashboardWindow.width, 1480)
            compare(dashboardWindow.height, 320)
            compare(dashboardWindow.visibility, Window.Windowed)
        }

        function test_pageSelectionAndBoundedNavigation() {
            compare(dashboardWindow.currentPageIndex, 0)

            keyClick(Qt.Key_Left)
            tryCompare(dashboardWindow, "currentPageIndex", 0)
            keyClick(Qt.Key_Right)
            tryCompare(dashboardWindow, "currentPageIndex", 1)
            keyClick(Qt.Key_Right)
            keyClick(Qt.Key_Right)
            keyClick(Qt.Key_Right)
            tryCompare(dashboardWindow, "currentPageIndex", 3)
            keyClick(Qt.Key_Home)
            tryCompare(dashboardWindow, "currentPageIndex", 0)
        }

        function test_touchTargetsSelectPages() {
            const buttonNames = ["overviewButton", "systemsButton", "projectsButton", "weatherButton"]
            for (let index = 0; index < buttonNames.length; ++index) {
                const button = findChild(dashboardWindow.contentItem, buttonNames[index])
                verify(!!button, "Object exists")
                compare(button.width, 56)
                compare(button.height, 56)
                mouseClick(button)
                tryCompare(dashboardWindow, "currentPageIndex", index)
            }
        }

        function test_f5FocusesCurrentPage() {
            keyClick(Qt.Key_Right)
            tryCompare(dashboardWindow, "currentPageIndex", 1)
            keyClick(Qt.Key_F5)
            tryCompare(dashboardWindow, "currentPageHasFocus", true)
        }

        function test_layoutStaysWithinTargetBounds() {
            const pageStack = findChild(dashboardWindow.contentItem, "pageStack")
            verify(!!pageStack, "Object exists")
            compare(pageStack.x >= 0, true)
            compare(pageStack.y >= 0, true)
            compare(pageStack.x + pageStack.width <= dashboardWindow.width, true)
            compare(pageStack.y + pageStack.height <= dashboardWindow.height, true)

            const pageNames = ["overviewPage", "systemsPage", "projectsPage", "weatherPage"]
            for (const pageName of pageNames) {
                const page = findChild(dashboardWindow.contentItem, pageName)
                verify(!!page, "Object exists")
                compare(page.x >= 0, true)
                compare(page.y >= 0, true)
                compare(page.x + page.width <= pageStack.width, true)
                compare(page.y + page.height <= pageStack.height, true)
            }
        }
    }
}
