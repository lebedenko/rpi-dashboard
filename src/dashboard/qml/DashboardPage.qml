import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: root

    required property string heading
    property alias placeholder: emptyState

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacingLarge
        anchors.topMargin: Math.max(Theme.spacingMedium, Theme.displaySafeInset)
        anchors.rightMargin: Math.max(Theme.spacingLarge, Theme.displaySafeInset)
        anchors.bottomMargin: Math.max(Theme.spacingLarge, Theme.displaySafeInset)
        spacing: Theme.spacingMedium

        Label {
            Layout.fillWidth: true
            text: root.heading
            color: Theme.textPrimary
            font.pixelSize: 28
            font.weight: Font.DemiBold
        }

        Control {
            id: emptyState

            Layout.fillWidth: true
            Layout.fillHeight: true
            activeFocusOnTab: true
            Accessible.role: Accessible.StaticText
            Accessible.name: qsTr("%1: Not implemented yet").arg(root.heading)

            contentItem: Label {
                text: qsTr("Not implemented yet")
                color: Theme.textSecondary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 22
            }

            background: Rectangle {
                color: Theme.surfaceElevated
                border.width: emptyState.activeFocus ? 2 : 1
                border.color: emptyState.activeFocus ? Theme.focusAccent : Theme.passiveBorder
                radius: 8

                Rectangle {
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 72
                    height: 3
                    color: Theme.primaryAccent
                    Accessible.ignored: true
                }
            }
        }
    }
}
