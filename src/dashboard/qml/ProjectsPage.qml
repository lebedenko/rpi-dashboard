pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import Rpi.Dashboard as Dashboard

FocusScope {
    id: root
    property var service: null
    readonly property alias focusTarget: projectList
    readonly property bool selectedRowHasFocus: projectList.currentItem ? projectList.currentItem.activeFocus : false
    readonly property alias projectListView: projectList

    function statusLabel(health): string {
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
    function available(value): string {
        return value === undefined || value === null || String(value).length === 0 ? "—" : String(value);
    }
    function selectCurrent(): void {
        if (root.service && projectList.currentIndex >= 0)
            root.service.selectProject(projectList.currentIndex);
    }
    function focusSelected(): void {
        if (projectList.currentItem)
            projectList.currentItem.forceActiveFocus();
        else
            projectList.forceActiveFocus();
    }

    Keys.onUpPressed: event => {
        projectList.decrementCurrentIndex();
        event.accepted = true;
    }
    Keys.onDownPressed: event => {
        projectList.incrementCurrentIndex();
        event.accepted = true;
    }
    Keys.onSpacePressed: event => {
        root.selectCurrent();
        event.accepted = true;
    }
    Keys.onReturnPressed: event => {
        root.selectCurrent();
        event.accepted = true;
    }
    Keys.onEnterPressed: event => {
        root.selectCurrent();
        event.accepted = true;
    }

    Connections {
        target: root.service
        function onSelectedProjectIndexChanged(): void {
            projectList.currentIndex = root.service ? root.service.selectedProjectIndex : -1;
        }
    }

    Dashboard.Frame {
        id: pageFrame
        objectName: "projectsPageFrame"
        anchors.fill: parent
        anchors.margins: Theme.spacingSmall
        backgroundColor: Theme.surface
        color: Theme.cardFrame
        lineWidth: 1
        corners: ({
                chamfered: Theme.chamferMedium
            })
    }
    Item {
        id: headerBand
        objectName: "projectsHeader"
        anchors.left: pageFrame.left
        anchors.right: pageFrame.right
        anchors.top: pageFrame.top
        height: 48
    }
    Text {
        id: heading
        objectName: "projectsHeading"
        anchors.left: headerBand.left
        anchors.leftMargin: 12
        anchors.verticalCenter: headerBand.verticalCenter
        color: Theme.primaryAccent
        font.family: Theme.sansFontFamily
        font.pixelSize: Theme.headingTextSize
        font.weight: Theme.headingFontWeight
        text: qsTr("Projects").toUpperCase()
    }
    Row {
        anchors.right: headerBand.right
        anchors.rightMargin: 12
        anchors.verticalCenter: headerBand.verticalCenter
        spacing: 5
        Text {
            objectName: "trackedNumber"
            color: Theme.textSecondary
            font.family: Theme.fixedFontFamily
            font.pixelSize: Theme.captionTextSize
            font.weight: Theme.technicalRegularFontWeight
            text: root.service ? root.service.trackedCount : "—"
        }
        Text {
            objectName: "trackedLabel"
            color: Theme.textSecondary
            font.family: Theme.sansFontFamily
            font.pixelSize: Theme.captionTextSize
            font.weight: Theme.labelFontWeight
            text: qsTr("TRACKED")
        }
        Text {
            color: Theme.textMuted
            font.family: Theme.sansFontFamily
            font.pixelSize: Theme.captionTextSize
            text: "·"
        }
        Text {
            objectName: "runningNumber"
            color: root.service && root.service.runningCount > 0 ? Theme.primaryAccent : Theme.textSecondary
            font.family: Theme.fixedFontFamily
            font.pixelSize: Theme.captionTextSize
            font.weight: Theme.technicalRegularFontWeight
            text: root.service ? root.service.runningCount : "—"
        }
        Text {
            objectName: "runningLabel"
            color: Theme.textSecondary
            font.family: Theme.sansFontFamily
            font.pixelSize: Theme.captionTextSize
            font.weight: Theme.labelFontWeight
            text: qsTr("RUNNING")
        }
        Text {
            color: Theme.textMuted
            font.family: Theme.sansFontFamily
            font.pixelSize: Theme.captionTextSize
            text: "·"
        }
        Text {
            objectName: "failedNumber"
            color: root.service && root.service.failedCount > 0 ? Theme.failureStatus : Theme.textSecondary
            font.family: Theme.fixedFontFamily
            font.pixelSize: Theme.captionTextSize
            font.weight: Theme.technicalRegularFontWeight
            text: root.service ? root.service.failedCount : "—"
        }
        Text {
            objectName: "failedLabel"
            color: root.service && root.service.failedCount > 0 ? Theme.failureStatus : Theme.textSecondary
            font.family: Theme.sansFontFamily
            font.pixelSize: Theme.captionTextSize
            font.weight: Theme.labelFontWeight
            text: qsTr("FAILED")
        }
    }

    Dashboard.Frame {
        id: listFrame
        objectName: "projectsListFrame"
        x: pageFrame.x + Theme.spacingSmall
        y: headerBand.y + headerBand.height
        width: 346
        height: pageFrame.height - headerBand.height - Theme.spacingSmall
        backgroundColor: Theme.cardSurface
        color: Theme.sectionDividerStrong
        lineWidth: 1
        corners: ({
                chamfered: Theme.chamferSmall
            })
    }
    ListView {
        id: projectList
        objectName: "projectList"
        anchors.fill: listFrame
        anchors.margins: Theme.spacingSmall
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
            required property string age
            required property string health
            readonly property color resolvedStatusColor: Theme.statusColor(projectRow.health)
            objectName: "projectRow" + index
            width: ListView.view.width
            height: 56
            padding: 0
            highlighted: ListView.isCurrentItem
            Accessible.name: qsTr("%1, %2, %3").arg(name).arg(branch).arg(root.statusLabel(health))
            Accessible.role: Accessible.ListItem
            onClicked: {
                ListView.view.currentIndex = index;
                root.service.selectProject(index);
                ListView.view.forceActiveFocus();
            }
            background: Dashboard.Frame {
                objectName: "projectCardFrame" + projectRow.index
                backgroundColor: projectRow.highlighted ? Theme.selectedSurface : Theme.cardSurface
                color: projectRow.highlighted ? Theme.primaryAccent : Theme.sectionDivider
                lineWidth: 1
                corners: ({
                        chamfered: Theme.chamferSmall
                    })
            }
            contentItem: Item {
                Rectangle {
                    objectName: "projectHealthRail" + projectRow.index
                    width: 4
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.spacingSmall
                    anchors.top: parent.top
                    anchors.topMargin: Theme.spacingSmall
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: Theme.spacingSmall
                    color: projectRow.resolvedStatusColor
                    Accessible.ignored: true
                }
                Row {
                    id: statusGroup
                    objectName: "projectStatusGroup" + projectRow.index
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.top: parent.top
                    anchors.topMargin: 7
                    spacing: 6
                    Item {
                        width: 7
                        height: statusText.height
                        Rectangle {
                            objectName: "projectStatusDisc" + projectRow.index
                            width: 7
                            height: 7
                            radius: width / 2
                            anchors.centerIn: parent
                            color: projectRow.resolvedStatusColor
                            Accessible.ignored: true
                        }
                    }
                    Text {
                        id: statusText
                        objectName: "projectHealth" + projectRow.index
                        color: projectRow.resolvedStatusColor
                        font.family: Theme.sansFontFamily
                        font.pixelSize: Theme.captionTextSize
                        font.weight: Theme.headingFontWeight
                        text: root.statusLabel(projectRow.health)
                    }
                }
                Text {
                    objectName: "projectName" + projectRow.index
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.spacingLarge
                    anchors.right: statusGroup.left
                    anchors.rightMargin: 12
                    anchors.top: parent.top
                    anchors.topMargin: 7
                    color: Theme.textPrimary
                    elide: Text.ElideRight
                    renderType: Text.NativeRendering
                    font.family: Theme.sansFontFamily
                    font.hintingPreference: Font.PreferFullHinting
                    font.pixelSize: Theme.bodyTextSize
                    font.weight: Theme.informationFontWeight
                    text: projectRow.name.toUpperCase()
                }
                Text {
                    objectName: "projectBranch" + projectRow.index
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.spacingLarge
                    anchors.right: projectAge.left
                    anchors.rightMargin: 12
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 6
                    color: Theme.textMuted
                    elide: Text.ElideRight
                    font.family: Theme.fixedFontFamily
                    font.pixelSize: Theme.captionTextSize
                    font.weight: Theme.technicalLightFontWeight
                    text: projectRow.branch.toLowerCase()
                }
                Text {
                    id: projectAge
                    objectName: "projectAge" + projectRow.index
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 6
                    color: Theme.textMuted
                    font.family: Theme.fixedFontFamily
                    font.pixelSize: Theme.captionTextSize
                    font.weight: Theme.technicalLightFontWeight
                    text: projectRow.age.toLowerCase()
                }
            }
        }
    }

    Dashboard.Frame {
        id: detail
        objectName: "projectDetail"
        anchors.left: listFrame.right
        anchors.leftMargin: Theme.spacingSmall
        anchors.right: pageFrame.right
        anchors.rightMargin: Theme.spacingSmall
        anchors.top: listFrame.top
        anchors.bottom: listFrame.bottom
        backgroundColor: Theme.surface
        color: Theme.sectionDividerStrong
        lineWidth: 1
        corners: ({
                chamfered: Theme.chamferSmall
            })
    }
    Item {
        id: detailContent
        anchors.fill: detail
        anchors.margins: 10
        Text {
            id: repositoryLabel
            objectName: "selectedRepository"
            anchors.left: parent.left
            anchors.top: parent.top
            color: Theme.textPrimary
            font.family: Theme.sansFontFamily
            font.pixelSize: Theme.metricTextSize
            font.weight: Theme.headingFontWeight
            text: root.service ? root.available(root.service.selectedRepository).toUpperCase() : "—"
        }
        Text {
            objectName: "selectedIdentity"
            anchors.left: repositoryLabel.right
            anchors.leftMargin: 9
            anchors.baseline: repositoryLabel.baseline
            color: Theme.textSecondary
            font.family: Theme.fixedFontFamily
            font.pixelSize: Theme.captionTextSize
            font.weight: Theme.technicalRegularFontWeight
            text: root.service ? qsTr("%1 · %2").arg(root.available(root.service.selectedBranch).toUpperCase()).arg(root.available(root.service.selectedRevision).toUpperCase()) : "—"
        }
        Text {
            objectName: "selectedHealth"
            anchors.left: parent.left
            anchors.top: repositoryLabel.bottom
            color: Theme.statusColor(root.service ? root.service.selectedHealth : "unknown")
            font.family: Theme.sansFontFamily
            font.pixelSize: Theme.captionTextSize
            font.weight: Theme.headingFontWeight
            text: root.statusLabel(root.service ? root.service.selectedHealth : "unknown")
        }
        Text {
            objectName: "selectedRun"
            anchors.right: parent.right
            anchors.top: parent.top
            color: Theme.textPrimary
            font.family: Theme.fixedFontFamily
            font.pixelSize: Theme.bodyTextSize
            font.weight: Theme.technicalFontWeight
            text: root.service ? root.available(root.service.selectedRun) : "—"
        }
        Text {
            objectName: "selectedRunAge"
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 20
            color: Theme.textMuted
            font.family: Theme.fixedFontFamily
            font.pixelSize: Theme.captionTextSize
            font.weight: Theme.technicalLightFontWeight
            text: root.service ? root.available(root.service.selectedRunAge).toLowerCase() : "—"
        }

        Row {
            id: stages
            objectName: "stageCards"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 43
            height: 58
            spacing: Theme.spacingSmall
            Repeater {
                model: root.service ? root.service.stageModel : null
                delegate: Item {
                    id: stageCard
                    required property int index
                    required property string name
                    required property string health
                    readonly property color resolvedStatusColor: Theme.statusColor(stageCard.health)
                    width: (stages.width - Theme.spacingLarge) / 4
                    height: stages.height
                    Dashboard.Frame {
                        objectName: "stageFrame" + stageCard.index
                        anchors.fill: parent
                        backgroundColor: Theme.cardSurface
                        color: stageCard.resolvedStatusColor
                        lineWidth: 1
                        corners: ({
                                chamfered: Theme.chamferSmall
                            })
                    }
                    Text {
                        objectName: "stageName" + stageCard.index
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 9
                        width: parent.width - Theme.spacingMedium
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                        font.family: Theme.sansFontFamily
                        font.pixelSize: Theme.bodyTextSize
                        font.weight: Theme.informationFontWeight
                        text: stageCard.name.toUpperCase()
                    }
                    Text {
                        objectName: "stageOutcome" + stageCard.index
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 7
                        color: stageCard.resolvedStatusColor
                        font.family: Theme.sansFontFamily
                        font.pixelSize: Theme.captionTextSize
                        font.weight: Theme.headingFontWeight
                        text: (stageCard.health === "healthy" ? "✓ " : "") + root.statusLabel(stageCard.health)
                    }
                    Rectangle {
                        visible: stageCard.index > 0
                        x: -8
                        anchors.verticalCenter: parent.verticalCenter
                        width: 8
                        height: 1
                        color: Theme.sectionDividerStrong
                        Accessible.ignored: true
                    }
                }
            }
        }
        Row {
            id: metrics
            anchors.left: stages.left
            anchors.right: stages.right
            anchors.top: stages.bottom
            anchors.topMargin: Theme.spacingSmall
            height: 44
            spacing: Theme.spacingSmall
            Repeater {
                model: [
                    {
                        "label": qsTr("DURATION"),
                        "value": root.service ? root.available(root.service.duration) : "—"
                    },
                    {
                        "label": qsTr("JOBS"),
                        "value": root.service ? root.available(root.service.jobsSummary) : "—"
                    },
                    {
                        "label": qsTr("ARTIFACTS"),
                        "value": root.service ? root.available(root.service.artifactSize) : "—"
                    },
                    {
                        "label": qsTr("DEPLOY"),
                        "value": root.service ? root.available(root.service.deployStatus) : "—"
                    }
                ]
                delegate: Item {
                    id: metricCard
                    required property int index
                    required property var modelData
                    width: (metrics.width - Theme.spacingLarge) / 4
                    height: metrics.height
                    Dashboard.Frame {
                        objectName: "metricFrame" + metricCard.index
                        anchors.fill: parent
                        backgroundColor: Theme.surfaceElevated
                        color: Theme.sectionDividerStrong
                        lineWidth: 1
                    }
                    Text {
                        objectName: "metricLabel" + metricCard.index
                        anchors.left: parent.left
                        anchors.leftMargin: Theme.spacingSmall
                        anchors.top: parent.top
                        anchors.topMargin: 5
                        color: Theme.textSecondary
                        font.family: Theme.sansFontFamily
                        font.pixelSize: Theme.captionTextSize
                        font.weight: Theme.labelFontWeight
                        text: metricCard.modelData.label
                    }
                    Text {
                        objectName: "metricValue" + metricCard.index
                        anchors.left: parent.left
                        anchors.leftMargin: Theme.spacingSmall
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 4
                        color: Theme.textPrimary
                        font.family: Theme.fixedFontFamily
                        font.pixelSize: Theme.captionTextSize
                        font.weight: Theme.technicalRegularFontWeight
                        text: metricCard.modelData.value
                    }
                }
            }
        }

        Text {
            id: historyHeading
            objectName: "historyHeading"
            anchors.left: parent.left
            anchors.bottom: historyStrip.top
            anchors.bottomMargin: 4
            color: Theme.textSecondary
            font.family: Theme.sansFontFamily
            font.pixelSize: Theme.captionTextSize
            font.weight: Theme.headingFontWeight
            text: qsTr("RUN HISTORY (%1)").arg(root.service ? root.service.historyCount : 0)
        }
        Text {
            objectName: "historySummary"
            anchors.right: parent.right
            anchors.baseline: historyHeading.baseline
            color: Theme.textSecondary
            font.family: Theme.fixedFontFamily
            font.pixelSize: Theme.captionTextSize
            font.weight: Theme.technicalRegularFontWeight
            text: qsTr("%1 / %2 SUCCESS").arg(root.service ? root.service.historySuccessfulCount : 0).arg(root.service ? root.service.historyCount : 0)
        }
        Row {
            id: historyStrip
            objectName: "runHistoryStrip"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 18
            spacing: 4
            Repeater {
                model: root.service ? root.service.runHistoryModel : null
                delegate: Rectangle {
                    required property string key
                    required property string health
                    width: Math.max(12, (historyStrip.width - 76) / 20)
                    height: historyStrip.height
                    color: Theme.statusColor(health)
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
            font.weight: Theme.informationFontWeight
            text: !root.service || root.service.state === "loading" ? qsTr("Loading GitHub projects…") : root.service.state === "error" ? qsTr("Projects unavailable: %1").arg(root.service.diagnostics) : qsTr("No repositories with default-branch runs")
        }
        Text {
            anchors.right: parent.right
            anchors.top: parent.top
            visible: root.service && root.service.stale
            color: Theme.staleStatus
            font.family: Theme.sansFontFamily
            font.pixelSize: Theme.captionTextSize
            font.weight: Theme.headingFontWeight
            text: qsTr("STALE · %1").arg(root.service.diagnostics)
        }
    }
}
