import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    required property string label
    required property string value
    property string detail: ""
    property color accent: Theme.interactive

    color: Theme.surface
    border.color: Theme.outline
    border.width: 1
    radius: 3

    Accessible.role: Accessible.StaticText
    Accessible.name: root.detail.length > 0
                     ? qsTr("%1: %2, %3").arg(root.label, root.value, root.detail)
                     : qsTr("%1: %2").arg(root.label, root.value)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingMedium
        spacing: Theme.spacingSmall

        Rectangle {
            Layout.preferredWidth: 32
            Layout.preferredHeight: 3
            color: root.accent
        }

        Text {
            Layout.fillWidth: true
            text: root.label
            color: Theme.textSecondary
            font.pixelSize: 16
            font.weight: Font.Medium
        }

        Item { Layout.fillHeight: true }

        Text {
            Layout.fillWidth: true
            text: root.value
            color: Theme.textPrimary
            font.pixelSize: 34
            font.weight: Font.Light
        }

        Text {
            Layout.fillWidth: true
            text: root.detail
            color: root.accent
            font.pixelSize: 14
            visible: text.length > 0
        }
    }
}

