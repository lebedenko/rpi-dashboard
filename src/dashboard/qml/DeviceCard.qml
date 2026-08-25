pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Control {
    id: root

    required property string deviceNumber
    required property string hostname
    required property bool online
    required property bool expanded
    required property real expandedHeight
    property string cpuMetric: "—"
    property string memoryMetric: "—"
    property real cpuUsageRatio: -1
    property real memoryUsageRatio: -1
    property var usageHistoryModel: null
    property string temperatureMetric: "—"
    property string uptimeMetric: "—"
    property string osDescription: "—"
    property string kernelDescription: "—"
    property string architecture: "—"
    property string hardwareDescription: "—"
    property string cpuDescription: "—"
    property string coreDescription: "—"
    property string totalMemory: "—"
    property bool selected: true
    readonly property alias chevronButton: chevron
    readonly property alias cpuProgressFill: cpuMetricCell.progressFill
    readonly property alias memoryProgressFill: memoryMetricCell.progressFill
    readonly property string chevronAccessibleName: root.expanded ? qsTr("Collapse %1").arg(root.hostname) : qsTr("Expand %1").arg(root.hostname)
    readonly property bool expandedContentLoaded: detailsLoader.status === Loader.Ready
    signal expansionRequested()
    signal selectionRequested()

    height: root.expanded ? root.expandedHeight : Theme.deviceHeaderHeight
    padding: Theme.cardFrameInset

    component UtilizationMetricCell: Item {
        required property string metricLabel
        required property string metricValue
        required property real usageRatio
        required property color seriesColor
        readonly property bool ratioAvailable: usageRatio >= 0
        readonly property alias progressFill: fill
        Rectangle { anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; width: 1; height: 34; color: Theme.sectionDivider }
        Text {
            anchors.left: parent.left; anchors.leftMargin: Theme.spacingMedium; anchors.top: parent.top; anchors.topMargin: 9
            text: parent.metricLabel; textFormat: Text.PlainText; color: Theme.textMuted
            font.family: Theme.fixedFontFamily; font.pixelSize: Theme.metricLabelTextSize; font.weight: Theme.technicalFontWeight
        }
        Text {
            anchors.right: parent.right; anchors.rightMargin: Theme.spacingMedium; anchors.top: parent.top; anchors.topMargin: 7
            text: parent.metricValue; textFormat: Text.PlainText; color: Theme.textSecondary
            font.family: Theme.fixedFontFamily; font.pixelSize: Theme.metricTextSize; font.weight: Theme.metricFontWeight
        }
        Rectangle {
            id: rail
            anchors.left: parent.left; anchors.leftMargin: Theme.spacingMedium
            anchors.right: parent.right; anchors.rightMargin: Theme.spacingMedium
            anchors.bottom: parent.bottom; anchors.bottomMargin: 10
            height: 8; color: Theme.metricRail; Accessible.ignored: true
            Rectangle {
                id: fill
                objectName: parent.parent.objectName + "ProgressFill"
                width: parent.width * Math.min(1, Math.max(0, parent.parent.usageRatio))
                height: parent.height; color: parent.parent.seriesColor
                visible: parent.parent.ratioAvailable
                Accessible.ignored: true
            }
        }
    }

    component StackedMetricCell: Item {
        required property string metricLabel
        required property string metricValue
        property color valueColor: Theme.textSecondary
        Rectangle { anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; width: 1; height: 34; color: Theme.sectionDivider }
        Text {
            anchors.left: parent.left; anchors.leftMargin: Theme.spacingMedium; anchors.top: parent.top; anchors.topMargin: 9
            text: parent.metricLabel; textFormat: Text.PlainText; color: Theme.textMuted
            font.family: Theme.fixedFontFamily; font.pixelSize: Theme.metricLabelTextSize; font.weight: Theme.technicalFontWeight
        }
        Text {
            anchors.left: parent.left; anchors.leftMargin: Theme.spacingMedium; anchors.bottom: parent.bottom; anchors.bottomMargin: 7
            text: parent.metricValue; textFormat: Text.PlainText; color: parent.valueColor
            font.family: Theme.fixedFontFamily; font.pixelSize: Theme.metricTextSize; font.weight: Theme.metricFontWeight
        }
    }

    component DetailRow: Item {
        required property url iconSource
        required property string detailLabel
        required property string detailValue
        readonly property int valuePixelSize: Theme.bodyTextSize
        Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 1; color: Theme.sectionDivider }
        Image {
            anchors.left: parent.left; anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter
            width: 16; height: 16; source: parent.iconSource; sourceSize.width: 16; sourceSize.height: 16; Accessible.ignored: true
        }
        Text {
            anchors.left: parent.left; anchors.leftMargin: Theme.detailIconRailWidth + 10; anchors.verticalCenter: parent.verticalCenter; width: 80
            text: parent.detailLabel; textFormat: Text.PlainText; color: Theme.textMuted
            font.family: Theme.fixedFontFamily; font.pixelSize: Theme.captionTextSize
        }
        Text {
            anchors.left: parent.left; anchors.leftMargin: Theme.detailIconRailWidth + 94; anchors.right: parent.right; anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter; text: parent.detailValue; textFormat: Text.PlainText
            color: Theme.textPrimary; elide: Text.ElideRight
            font.family: Theme.sansFontFamily; font.pixelSize: parent.valuePixelSize; font.weight: Theme.informationFontWeight
        }
    }

    component FooterButton: Button {
        required property url actionIcon
        property bool selectedStyle: false
        readonly property bool selectedSurfaceVisible: selectedStyle
        readonly property bool disabledSurfaceVisible: !enabled
        checkable: false
        checked: selectedStyle
        Accessible.name: text
        contentItem: Row {
            spacing: 10; anchors.centerIn: parent
            Image {
                width: 20; height: 20; source: parent.parent.actionIcon; sourceSize.width: 20; sourceSize.height: 20
                opacity: parent.parent.selectedStyle ? 1 : 0.58; Accessible.ignored: true
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter; text: parent.parent.text; textFormat: Text.PlainText
                color: parent.parent.selectedStyle ? Theme.selectedActionContent : Theme.disabledActionContent
                font.family: Theme.fixedFontFamily; font.pixelSize: Theme.actionTextSize; font.weight: Theme.technicalFontWeight
            }
        }
        background: Shape {
            ShapePath {
                fillColor: parent.selectedStyle ? Theme.selectedActionSurface : Theme.disabledActionSurface
                strokeColor: parent.selectedStyle ? Theme.selectedActionFrame : Theme.disabledActionFrame
                strokeWidth: 1; joinStyle: ShapePath.MiterJoin; startX: 0; startY: Theme.actionChamfer
                PathLine { x: Theme.actionChamfer; y: 0 }
                PathLine { x: parent.width; y: 0 }
                PathLine { x: parent.width; y: parent.height - Theme.actionChamfer }
                PathLine { x: parent.width - Theme.actionChamfer; y: parent.height }
                PathLine { x: 0; y: parent.height }
                PathLine { x: 0; y: Theme.actionChamfer }
            }
        }
    }

    background: Item {
        Shape {
            id: outerFrame
            anchors.fill: parent; anchors.margins: Theme.cardFrameInset; Accessible.ignored: true
            ShapePath {
                fillColor: Theme.cardSurface; strokeColor: Theme.cardFrame; strokeWidth: 1; joinStyle: ShapePath.MiterJoin
                startX: Theme.deviceFrameStep; startY: 0
                PathLine { x: outerFrame.width - Theme.deviceFrameChamfer; y: 0 }
                PathLine { x: outerFrame.width; y: Theme.deviceFrameChamfer }
                PathLine { x: outerFrame.width; y: outerFrame.height - Theme.deviceFrameStep }
                PathLine { x: outerFrame.width - Theme.deviceFrameStep; y: outerFrame.height }
                PathLine { x: Theme.deviceFrameChamfer; y: outerFrame.height }
                PathLine { x: 0; y: outerFrame.height - Theme.deviceFrameChamfer }
                PathLine { x: 0; y: Theme.deviceFrameStep }
                PathLine { x: Theme.deviceFrameStep; y: 0 }
            }
        }
        Rectangle { x: Theme.deviceFrameStep; y: Theme.cardFrameInset; width: Math.min(150, parent.width * 0.13); height: 2; color: Theme.cardAccent }
        Rectangle { x: parent.width - width - Theme.deviceFrameStep; y: parent.height - Theme.cardFrameInset - 2; width: Math.min(128, parent.width * 0.11); height: 2; color: Theme.cardAccent }
    }

    contentItem: Item {
        Item {
            id: header
            objectName: "deviceHeader"
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
            height: Theme.deviceHeaderHeight - 2 * Theme.cardFrameInset
            Text {
                id: numberLabel
                objectName: "deviceNumber"
                anchors.left: parent.left; anchors.leftMargin: Theme.spacingLarge; anchors.verticalCenter: parent.verticalCenter
                text: root.deviceNumber; textFormat: Text.PlainText; color: Theme.cardAccent
                font.family: Theme.fixedFontFamily; font.pixelSize: Theme.headingTextSize; font.weight: Theme.technicalFontWeight
            }
            Rectangle { anchors.left: numberLabel.right; anchors.leftMargin: Theme.spacingMedium; anchors.verticalCenter: parent.verticalCenter; width: 1; height: 36; color: Theme.sectionDivider }
            Text {
                objectName: "deviceHostname"
                anchors.left: numberLabel.right; anchors.leftMargin: 34; anchors.verticalCenter: parent.verticalCenter; width: 190
                text: root.hostname; textFormat: Text.PlainText; color: Theme.textPrimary; elide: Text.ElideRight
                font.family: Theme.sansFontFamily; font.pixelSize: Theme.headingTextSize; font.weight: Theme.headingFontWeight
            }
            Shape {
                id: statusBadge
                x: 314; anchors.verticalCenter: parent.verticalCenter; width: Theme.statusBadgeWidth; height: Theme.statusBadgeHeight; Accessible.ignored: true
                ShapePath {
                    fillColor: Theme.badgeSurface; strokeColor: root.online ? Theme.onlineFrame : Theme.disabledActionFrame; strokeWidth: 1
                    startX: 0; startY: 0
                    PathLine { x: statusBadge.width - Theme.badgeChamfer; y: 0 }
                    PathLine { x: statusBadge.width; y: statusBadge.height / 2 }
                    PathLine { x: statusBadge.width - Theme.badgeChamfer; y: statusBadge.height }
                    PathLine { x: 0; y: statusBadge.height }
                    PathLine { x: 0; y: 0 }
                }
                Rectangle { x: 15; anchors.verticalCenter: parent.verticalCenter; width: 10; height: 10; radius: 5; color: root.online ? Theme.onlineStatus : Theme.textMuted }
                Text {
                    objectName: "deviceStatus"; x: 34; anchors.verticalCenter: parent.verticalCenter
                    text: root.online ? qsTr("ONLINE") : qsTr("OFFLINE"); textFormat: Text.PlainText
                    color: root.online ? Theme.onlineStatus : Theme.textMuted
                    font.family: Theme.fixedFontFamily; font.pixelSize: Theme.bodyTextSize; font.weight: Theme.technicalFontWeight
                }
            }
            Item {
                id: metrics
                anchors.left: statusBadge.right; anchors.leftMargin: Theme.spacingLarge; anchors.right: chevron.left; anchors.rightMargin: Theme.spacingMedium
                anchors.top: parent.top; anchors.bottom: parent.bottom
                UtilizationMetricCell {
                    id: cpuMetricCell
                    objectName: "cpuMetricCell"
                    x: 0; width: parent.width * 0.28; height: parent.height
                    metricLabel: qsTr("CPU"); metricValue: root.cpuMetric; usageRatio: root.cpuUsageRatio; seriesColor: Theme.cpuSeries
                }
                UtilizationMetricCell {
                    id: memoryMetricCell
                    objectName: "memoryMetricCell"
                    x: parent.width * 0.28; width: parent.width * 0.28; height: parent.height
                    metricLabel: qsTr("MEM"); metricValue: root.memoryMetric; usageRatio: root.memoryUsageRatio; seriesColor: Theme.memorySeries
                }
                StackedMetricCell {
                    objectName: "temperatureMetricCell"
                    x: parent.width * 0.56; width: parent.width * 0.18; height: parent.height
                    metricLabel: qsTr("TEMP"); metricValue: root.temperatureMetric; valueColor: Theme.cpuSeries
                }
                StackedMetricCell {
                    objectName: "uptimeMetricCell"
                    x: parent.width * 0.74; width: parent.width * 0.26; height: parent.height
                    metricLabel: qsTr("UPTIME"); metricValue: root.uptimeMetric
                }
            }
            Button {
                id: chevron
                objectName: "deviceChevron"
                anchors.right: parent.right; anchors.rightMargin: Theme.spacingSmall; anchors.verticalCenter: parent.verticalCenter
                width: 48; height: 48; activeFocusOnTab: true; display: AbstractButton.IconOnly
                icon.source: Qt.resolvedUrl("icons/chevron.svg"); icon.color: activeFocus ? Theme.focusAccent : Theme.cardAccent
                icon.width: 24; icon.height: 24; rotation: root.expanded ? 180 : 0
                Accessible.name: root.chevronAccessibleName
                onClicked: root.expansionRequested()
                background: Rectangle {
                    color: chevron.down ? Theme.surfaceRaised : "transparent"
                    border.width: chevron.activeFocus ? 2 : 1; border.color: chevron.activeFocus ? Theme.focusAccent : Theme.passiveBorder; radius: 2
                }
            }
        }
        Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.top: header.bottom; height: 1; color: Theme.sectionDivider }
        Loader {
            id: detailsLoader
            objectName: "expandedContentLoader"
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: header.bottom; anchors.bottom: parent.bottom
            active: root.expanded; sourceComponent: expandedContent
        }
    }

    Component {
        id: expandedContent
        Item {
            Item {
                id: body
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: footerSeparator.top
                anchors.margins: Theme.spacingSmall
                Item {
                    id: details
                    objectName: "deviceDetails"
                    anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: (body.width - Theme.sectionGap) * Theme.detailsWidthRatio
                    Text {
                        anchors.left: parent.left; anchors.leftMargin: Theme.detailIconRailWidth + 10; anchors.top: parent.top
                        text: qsTr("DEVICE DETAILS"); textFormat: Text.PlainText; color: Theme.cardAccent
                        font.family: Theme.sansFontFamily; font.pixelSize: Theme.sectionTitleTextSize; font.weight: Theme.technicalFontWeight
                    }
                    Rectangle {
                        anchors.left: parent.left; anchors.top: parent.top; anchors.topMargin: Theme.detailHeaderHeight; anchors.bottom: parent.bottom
                        width: Theme.detailIconRailWidth; color: Theme.detailRailSurface; border.width: 1; border.color: Theme.detailRailFrame
                    }
                    DetailRow { objectName: "detailRow0"; x: 0; y: Theme.detailHeaderHeight; width: parent.width; height: (parent.height - Theme.detailHeaderHeight) / 7; iconSource: Qt.resolvedUrl("icons/detail-os.svg"); detailLabel: qsTr("OS"); detailValue: root.osDescription }
                    DetailRow { objectName: "detailRow1"; x: 0; y: Theme.detailHeaderHeight + height; width: parent.width; height: (parent.height - Theme.detailHeaderHeight) / 7; iconSource: Qt.resolvedUrl("icons/detail-kernel.svg"); detailLabel: qsTr("KERNEL"); detailValue: root.kernelDescription }
                    DetailRow { objectName: "detailRow2"; x: 0; y: Theme.detailHeaderHeight + height * 2; width: parent.width; height: (parent.height - Theme.detailHeaderHeight) / 7; iconSource: Qt.resolvedUrl("icons/detail-arch.svg"); detailLabel: qsTr("ARCH"); detailValue: root.architecture }
                    DetailRow { objectName: "detailRow3"; x: 0; y: Theme.detailHeaderHeight + height * 3; width: parent.width; height: (parent.height - Theme.detailHeaderHeight) / 7; iconSource: Qt.resolvedUrl("icons/detail-hardware.svg"); detailLabel: qsTr("HARDWARE"); detailValue: root.hardwareDescription }
                    DetailRow { objectName: "detailRow4"; x: 0; y: Theme.detailHeaderHeight + height * 4; width: parent.width; height: (parent.height - Theme.detailHeaderHeight) / 7; iconSource: Qt.resolvedUrl("icons/detail-cpu.svg"); detailLabel: qsTr("PROCESSOR"); detailValue: root.cpuDescription }
                    DetailRow { objectName: "detailRow5"; x: 0; y: Theme.detailHeaderHeight + height * 5; width: parent.width; height: (parent.height - Theme.detailHeaderHeight) / 7; iconSource: Qt.resolvedUrl("icons/detail-cores.svg"); detailLabel: qsTr("CORES"); detailValue: root.coreDescription }
                    DetailRow { objectName: "detailRow6"; x: 0; y: Theme.detailHeaderHeight + height * 6; width: parent.width; height: (parent.height - Theme.detailHeaderHeight) / 7; iconSource: Qt.resolvedUrl("icons/detail-memory.svg"); detailLabel: qsTr("MEMORY"); detailValue: root.totalMemory }
                }
                Rectangle {
                    id: bodyDivider
                    anchors.left: details.right; anchors.leftMargin: Theme.sectionGap / 2; anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: 1; color: Theme.sectionDividerStrong
                }
                Item {
                    id: history
                    objectName: "resourceHistory"
                    anchors.left: bodyDivider.right; anchors.leftMargin: Theme.sectionGap / 2; anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
                    Text {
                        anchors.left: parent.left; anchors.top: parent.top; text: qsTr("RESOURCE HISTORY"); textFormat: Text.PlainText; color: Theme.cardAccent
                        font.family: Theme.sansFontFamily; font.pixelSize: Theme.sectionTitleTextSize; font.weight: Theme.technicalFontWeight
                    }
                    Row {
                        anchors.right: parent.right; anchors.top: parent.top; spacing: 18
                        Row { spacing: 6; Rectangle { objectName: "cpuHistoryLegendLine"; anchors.verticalCenter: parent.verticalCenter; width: 14; height: 2; color: Theme.cpuSeries } Text { objectName: "cpuHistoryLegend"; text: qsTr("CPU %"); textFormat: Text.PlainText; color: Theme.chartText; font.family: Theme.fixedFontFamily; font.pixelSize: Theme.captionTextSize } }
                        Row { spacing: 6; Rectangle { objectName: "memoryHistoryLegendLine"; anchors.verticalCenter: parent.verticalCenter; width: 14; height: 2; color: Theme.memorySeries } Text { objectName: "memoryHistoryLegend"; text: qsTr("MEM %"); textFormat: Text.PlainText; color: Theme.chartText; font.family: Theme.fixedFontFamily; font.pixelSize: Theme.captionTextSize } }
                    }
                    Item {
                        id: plot
                        objectName: "historyGrid"
                        clip: true
                        anchors.fill: parent
                        anchors.leftMargin: Theme.plotLeftPadding; anchors.rightMargin: Theme.plotRightPadding
                        anchors.topMargin: Theme.plotTopPadding; anchors.bottomMargin: Theme.plotBottomPadding
                        Repeater {
                            model: 4
                            delegate: Rectangle {
                                required property int index
                                objectName: "historyHorizontalGuide" + index
                                y: index * (plot.height - 1) / 4
                                width: plot.width
                                height: 1
                                color: Theme.chartGrid
                            }
                        }
                        Rectangle { objectName: "historyBaseline"; y: parent.height - 1; width: parent.width; height: 1; color: Theme.chartAxis }
                        ResourceHistorySeries {
                            id: historySeries
                            objectName: "resourceHistorySeries"
                            anchors.fill: parent
                            model: root.usageHistoryModel
                            cpuColor: Theme.cpuSeries
                            memoryColor: Theme.memorySeries
                            plotBackgroundColor: Theme.cardSurface
                            transitionDuration: 350
                            Accessible.ignored: true
                        }
                    }
                    Repeater {
                        model: 5
                        delegate: Text {
                            required property int index
                            objectName: "historyValue" + index
                            x: 0; y: Theme.plotTopPadding + index * (plot.height - height) / 4; width: Theme.plotLeftPadding - 6
                            horizontalAlignment: Text.AlignRight; text: String(100 - index * 25); textFormat: Text.PlainText
                            color: Theme.chartText; font.family: Theme.fixedFontFamily; font.pixelSize: Theme.axisTextSize
                        }
                    }
                    Repeater {
                        model: [qsTr("−60s"), qsTr("−45s"), qsTr("−30s"), qsTr("−15s"), qsTr("NOW")]
                        delegate: Text {
                            id: timeLabel
                            required property int index
                            required property string modelData
                            objectName: "historyLabel" + timeLabel.index
                            x: timeLabel.index === 0 ? Theme.plotLeftPadding
                               : timeLabel.index === 4 ? history.width - Theme.plotRightPadding - width
                               : Theme.plotLeftPadding + timeLabel.index * plot.width / 4 - width / 2
                            y: history.height - height; text: timeLabel.modelData; textFormat: Text.PlainText
                            color: Theme.chartText; font.family: Theme.fixedFontFamily; font.pixelSize: Theme.axisTextSize
                        }
                    }
                }
            }
            Rectangle { id: footerSeparator; anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: footer.top; height: 1; color: Theme.sectionDividerStrong }
            Row {
                id: footer
                objectName: "deviceFooter"
                anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                height: Theme.deviceFooterHeight; spacing: Theme.footerGap
                FooterButton { id: selectActive; objectName: "selectActiveButton"; width: (footer.width - footer.spacing * 3) / 4; height: footer.height; text: qsTr("SELECT ACTIVE"); actionIcon: Qt.resolvedUrl("icons/check.svg"); selectedStyle: root.selected; onClicked: root.selectionRequested() }
                FooterButton { objectName: "viewStreamsButton"; width: (footer.width - footer.spacing * 3) / 4; height: footer.height; text: qsTr("VIEW STREAMS"); actionIcon: Qt.resolvedUrl("icons/streams.svg"); enabled: false }
                FooterButton { objectName: "terminalButton"; width: (footer.width - footer.spacing * 3) / 4; height: footer.height; text: qsTr("TERMINAL"); actionIcon: Qt.resolvedUrl("icons/terminal.svg"); enabled: false }
                FooterButton { objectName: "moreButton"; width: (footer.width - footer.spacing * 3) / 4; height: footer.height; text: qsTr("MORE"); actionIcon: Qt.resolvedUrl("icons/more.svg"); enabled: false }
            }
        }
    }
}
