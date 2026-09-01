import QtQuick

Item {
    id: root

    property real position: 0.5
    property bool knobVisible: true

    Rectangle {
        anchors.left: parent.left
        anchors.right: knob.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        height: 2

        gradient: Gradient {
            orientation: Gradient.Horizontal

            GradientStop {
                position: 0
                color: Theme.blueAccent
            }

            GradientStop {
                position: 1
                color: Theme.textPrimary
            }
        }
    }

    Rectangle {
        anchors.left: knob.horizontalCenter
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: 2

        gradient: Gradient {
            orientation: Gradient.Horizontal

            GradientStop {
                position: 0
                color: Theme.textPrimary
            }

            GradientStop {
                position: 1
                color: Theme.attentionStatus
            }
        }
    }

    Rectangle {
        id: knob

        objectName: "temperatureKnob"
        x: Math.max(0, Math.min(parent.width - width, root.position * parent.width - width / 2))
        anchors.verticalCenter: parent.verticalCenter
        width: 7
        height: 7
        radius: width / 2
        color: Theme.textPrimary
        visible: root.knobVisible
    }
}
