pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic

FocusScope {
    id: root

    property var service: null
    readonly property alias focusTarget: projectList
    readonly property bool selectedRowHasFocus: projectList.currentItem ? projectList.currentItem.activeFocus : false
    readonly property alias projectListView: projectList

    function statusLabel(health): string {
        const labels = {"failed": qsTr("FAILED"), "attention": qsTr("ATTENTION"),
                        "running": qsTr("RUNNING"), "stale": qsTr("STALE"),
                        "healthy": qsTr("HEALTHY"), "unknown": qsTr("UNKNOWN")}
        return labels[health] || labels.unknown
    }

    function statusColor(health): color {
        const colors = {"failed": Theme.failureStatus, "attention": Theme.attentionStatus,
                        "running": Theme.runningStatus, "stale": Theme.staleStatus,
                        "healthy": Theme.healthyStatus, "unknown": Theme.unknownStatus}
        return colors[health] || Theme.unknownStatus
    }

    function available(value): string {
        return value === undefined || value === null || String(value).length === 0 ? "—" : String(value)
    }

    function selectCurrent(): void {
        if (root.service && projectList.currentIndex >= 0)
            root.service.selectProject(projectList.currentIndex)
    }

    function focusSelected(): void {
        if (projectList.currentItem)
            projectList.currentItem.forceActiveFocus()
        else
            projectList.forceActiveFocus()
    }

    Keys.onUpPressed: event => {
        projectList.decrementCurrentIndex()
        event.accepted = true
    }
    Keys.onDownPressed: event => {
        projectList.incrementCurrentIndex()
        event.accepted = true
    }
    Keys.onSpacePressed: event => {
        root.selectCurrent()
        event.accepted = true
    }
    Keys.onReturnPressed: event => {
        root.selectCurrent()
        event.accepted = true
    }
    Keys.onEnterPressed: event => {
        root.selectCurrent()
        event.accepted = true
    }

    Text {
        id: heading
        objectName: "projectsHeading"
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.top: parent.top
        anchors.topMargin: 4
        color: Theme.textPrimary
        font.family: Theme.sansFontFamily
        font.pixelSize: Theme.headingTextSize
        font.weight: Theme.headingFontWeight
        text: qsTr("PROJECT HEALTH")
    }

    Text {
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: heading.verticalCenter
        color: Theme.textMuted
        font.family: Theme.fixedFontFamily
        font.pixelSize: Theme.captionTextSize
        text: root.service ? qsTr("%1 TRACKED · %2 RUNNING · %3 FAILED")
                                 .arg(root.service.trackedCount).arg(root.service.runningCount).arg(root.service.failedCount)
                           : qsTr("— TRACKED")
    }

    ListView {
        id: projectList
        objectName: "projectList"
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.top: parent.top
        anchors.topMargin: 38
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        width: 330
        model: root.service ? root.service.projectModel : null
        currentIndex: root.service ? root.service.selectedProjectIndex : -1
        spacing: 6
        clip: true
        reuseItems: true
        activeFocusOnTab: true
        Accessible.role: Accessible.List
        Accessible.name: qsTr("Projects")

        delegate: ItemDelegate {
            id: projectRow
            required property int index
            required property string name
            required property string branch
            required property string health
            required property string status

            objectName: "projectRow" + index
            width: ListView.view.width
            height: 56
            padding: 0
            highlighted: ListView.isCurrentItem
            Accessible.name: qsTr("%1, %2, %3").arg(name).arg(branch).arg(root.statusLabel(health))
            Accessible.role: Accessible.ListItem
            onClicked: {
                ListView.view.currentIndex = index
                root.selectCurrent()
                ListView.view.forceActiveFocus()
            }

            background: Rectangle {
                color: projectRow.highlighted ? Theme.selectedSurface : Theme.cardSurface
                border.color: projectRow.highlighted ? root.statusColor(projectRow.health) : Theme.cardFrame
                border.width: 1
                Rectangle {
                    width: 4
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    color: root.statusColor(projectRow.health)
                    Accessible.ignored: true
                }
            }

            contentItem: Item {
                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.statusColor(projectRow.health)
                    Accessible.ignored: true
                }
                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 36
                    anchors.top: parent.top
                    anchors.topMargin: 8
                    width: 180
                    color: Theme.textPrimary
                    elide: Text.ElideRight
                    font.family: Theme.sansFontFamily
                    font.pixelSize: Theme.bodyTextSize
                    font.weight: Theme.informationFontWeight
                    text: projectRow.name
                }
                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 36
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 7
                    width: 180
                    color: Theme.textMuted
                    elide: Text.ElideRight
                    font.family: Theme.fixedFontFamily
                    font.pixelSize: Theme.captionTextSize
                    text: projectRow.branch
                }
                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.statusColor(projectRow.health)
                    font.family: Theme.fixedFontFamily
                    font.pixelSize: Theme.captionTextSize
                    text: root.statusLabel(projectRow.health)
                }
            }
        }
    }

    Rectangle {
        id: detail
        objectName: "projectDetail"
        anchors.left: projectList.right
        anchors.leftMargin: 8
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.top: projectList.top
        anchors.bottom: projectList.bottom
        color: Theme.surface
        border.color: Theme.cardFrame

        Text {
            id: repositoryLabel
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.top: parent.top
            anchors.topMargin: 10
            color: Theme.textPrimary
            font.family: Theme.sansFontFamily
            font.pixelSize: Theme.metricTextSize
            font.weight: Theme.headingFontWeight
            text: root.service ? root.available(root.service.selectedRepository) : "—"
        }
        Text {
            anchors.left: repositoryLabel.left
            anchors.top: repositoryLabel.bottom
            color: Theme.textSecondary
            font.family: Theme.fixedFontFamily
            font.pixelSize: Theme.captionTextSize
            text: root.service ? qsTr("%1 · %2 · %3 · %4")
                                 .arg(root.available(root.service.selectedBranch))
                                 .arg(root.available(root.service.selectedRevision))
                                 .arg(root.available(root.service.selectedRun))
                                 .arg(root.available(root.service.selectedRunAge)) : "—"
        }

        Row {
            id: stages
            objectName: "stageCards"
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.top: parent.top
            anchors.topMargin: 58
            height: 62
            spacing: 8
            Repeater {
                model: root.service ? root.service.stageModel : null
                delegate: Rectangle {
                    required property string name
                    required property string health
                    width: (stages.width - 24) / 4
                    height: 62
                    color: Theme.cardSurface
                    border.color: root.statusColor(health)
                    Text {
                        anchors.centerIn: parent
                        width: parent.width - 12
                        color: root.statusColor(health)
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                        font.family: Theme.sansFontFamily
                        font.pixelSize: Theme.bodyTextSize
                        text: name
                    }
                }
            }
        }

        Row {
            anchors.left: stages.left
            anchors.right: stages.right
            anchors.top: stages.bottom
            anchors.topMargin: 10
            height: 52
            spacing: 8
            Repeater {
                model: [qsTr("DURATION|%1").arg(root.service ? root.available(root.service.duration) : "—"),
                        qsTr("JOBS|%1").arg(root.service ? root.available(root.service.jobsSummary) : "—"),
                        qsTr("ARTIFACTS|%1").arg(root.service ? root.available(root.service.artifactSize) : "—"),
                        qsTr("DEPLOY|%1").arg(root.service ? root.available(root.service.deployStatus).toUpperCase() : "—")]
                delegate: Rectangle {
                    required property string modelData
                    width: (stages.width - 24) / 4
                    height: 52
                    color: Theme.surfaceElevated
                    border.color: Theme.sectionDividerStrong
                    Text {
                        anchors.centerIn: parent
                        color: Theme.textSecondary
                        font.family: Theme.fixedFontFamily
                        font.pixelSize: Theme.captionTextSize
                        horizontalAlignment: Text.AlignHCenter
                        text: modelData.replace("|", "\n")
                    }
                }
            }
        }

        Row {
            id: historyStrip
            objectName: "runHistoryStrip"
            anchors.left: stages.left
            anchors.right: stages.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            height: 22
            spacing: 5
            Repeater {
                model: root.service ? root.service.runHistoryModel : null
                delegate: Rectangle {
                    required property string key
                    required property string health
                    width: Math.max(14, (historyStrip.width - 95) / 20)
                    height: 22
                    color: root.statusColor(health)
                    Accessible.role: Accessible.StaticText
                    Accessible.name: qsTr("Run %1, %2").arg(key).arg(root.statusLabel(health))
                }
            }
        }

        Text {
            anchors.centerIn: parent
            visible: !root.service || root.service.state !== "ready" || root.service.trackedCount === 0
            color: root.service && root.service.state === "error" ? Theme.failureStatus : Theme.textSecondary
            font.family: Theme.sansFontFamily
            font.pixelSize: Theme.bodyTextSize
            text: !root.service || root.service.state === "loading" ? qsTr("Loading GitHub projects…")
                  : root.service.state === "error" ? qsTr("Projects unavailable: %1").arg(root.service.diagnostics)
                  : qsTr("No repositories with default-branch runs")
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.top: parent.top
            anchors.topMargin: 10
            visible: root.service && root.service.stale
            color: Theme.staleStatus
            font.family: Theme.fixedFontFamily
            font.pixelSize: Theme.captionTextSize
            text: qsTr("STALE · %1").arg(root.service.diagnostics)
        }
    }
}
