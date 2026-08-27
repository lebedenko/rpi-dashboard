pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: root

    required property var deviceModel
    property var usageHistoryModel: null
    property int expandedIndex: 0
    required property int selectedIndex
    signal selectionRequested(int index)
    property int focusedIndex: 0
    property bool modelInitialized: false
    property int expandedIndexBeforeModelReset: 0
    readonly property Item currentCard: deviceList.currentItem
    readonly property var focusTarget: root.currentCard ? root.currentCard.chevronButton : null
    readonly property alias deviceList: deviceList

    function normalizedIndex(index: int): int {
        return deviceList.count > 0 ? Math.max(0, Math.min(index, deviceList.count - 1)) : -1
    }

    function normalizeIndices(): void {
        root.focusedIndex = root.normalizedIndex(root.focusedIndex)
        const selected = root.normalizedIndex(root.selectedIndex)
        if (selected !== root.selectedIndex)
            root.selectionRequested(selected)
        if (deviceList.count === 0) {
            if (root.expandedIndex >= 0)
                root.expandedIndexBeforeModelReset = root.expandedIndex
            if (root.modelInitialized)
                root.expandedIndex = -1
            return
        }
        if (root.expandedIndexBeforeModelReset >= 0 && root.expandedIndex < 0) {
            root.expandedIndex = root.normalizedIndex(root.expandedIndexBeforeModelReset)
            root.expandedIndexBeforeModelReset = -1
        } else if (root.expandedIndex >= deviceList.count) {
            root.expandedIndex = deviceList.count - 1
        }
    }

    function expandCard(index: int): void {
        const normalized = root.normalizedIndex(index)
        if (normalized < 0)
            return
        root.focusedIndex = normalized
        root.expandedIndex = normalized
        root.selectionRequested(normalized)
        Qt.callLater(() => {
            deviceList.forceLayout()
            deviceList.positionViewAtIndex(normalized, ListView.Beginning)
        })
    }

    Connections {
        target: root.deviceModel
        ignoreUnknownSignals: true
        function onCountChanged(): void { root.normalizeIndices() }
    }

    ListView {
        id: deviceList
        objectName: "deviceList"
        anchors.fill: parent
        anchors.margins: Theme.displaySafeInset
        spacing: Theme.deviceCardGap
        clip: true
        interactive: contentHeight > height
        boundsBehavior: Flickable.StopAtBounds
        model: root.deviceModel
        currentIndex: root.focusedIndex
        onCountChanged: {
            if (root.modelInitialized)
                root.normalizeIndices()
        }
        delegate: Item {
            id: cardDelegate
            required property int index
            required property string deviceNumber
            required property string hostname
            required property string statusKey
            required property bool historyAvailable
            required property string cpuMetric
            required property string memoryMetric
            required property real cpuUsageRatio
            required property real memoryUsageRatio
            required property string temperatureMetric
            required property string uptimeMetric
            required property string osDescription
            required property string kernelDescription
            required property string architecture
            required property string hardwareDescription
            required property string cpuDescription
            required property string coreDescription
            required property string totalMemory
            readonly property alias chevronButton: card.chevronButton
            readonly property alias expandedContentLoaded: card.expandedContentLoaded
            readonly property alias chevronAccessibleName: card.chevronAccessibleName
            readonly property alias expanded: card.expanded
            readonly property alias selected: card.selected
            readonly property alias online: card.online
            readonly property alias availableWidth: card.availableWidth
            readonly property alias cpuProgressFill: card.cpuProgressFill
            readonly property alias memoryProgressFill: card.memoryProgressFill

            objectName: "deviceCard" + cardDelegate.index
            width: ListView.view.width
            height: card.height

            DeviceCard {
                id: card
                width: parent.width
                deviceNumber: cardDelegate.deviceNumber
                hostname: cardDelegate.hostname
                statusKey: cardDelegate.statusKey
                historyAvailable: cardDelegate.historyAvailable
                selected: root.selectedIndex === cardDelegate.index
                cpuMetric: cardDelegate.cpuMetric
                memoryMetric: cardDelegate.memoryMetric
                cpuUsageRatio: cardDelegate.cpuUsageRatio
                memoryUsageRatio: cardDelegate.memoryUsageRatio
                usageHistoryModel: root.usageHistoryModel
                temperatureMetric: cardDelegate.temperatureMetric
                uptimeMetric: cardDelegate.uptimeMetric
                expanded: root.expandedIndex === cardDelegate.index
                expandedHeight: cardDelegate.index < deviceList.count - 1
                                ? deviceList.height - Theme.deviceCardGap - Theme.nextCardPeek
                                : deviceList.height
                osDescription: cardDelegate.osDescription
                kernelDescription: cardDelegate.kernelDescription
                architecture: cardDelegate.architecture
                hardwareDescription: cardDelegate.hardwareDescription
                cpuDescription: cardDelegate.cpuDescription
                coreDescription: cardDelegate.coreDescription
                totalMemory: cardDelegate.totalMemory
                onExpansionRequested: {
                    root.focusedIndex = cardDelegate.index
                    if (card.expanded) {
                        root.expandedIndexBeforeModelReset = -1
                        root.expandedIndex = -1
                    } else {
                        root.expandCard(cardDelegate.index)
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        root.modelInitialized = true
        root.normalizeIndices()
    }
}
