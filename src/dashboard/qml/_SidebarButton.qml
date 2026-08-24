import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Button {
    id: root

    property bool selected: false
    property url iconSource
    property string tooltipText

    display: AbstractButton.IconOnly
    icon.source: root.iconSource
    icon.width: 24
    icon.height: 24
    icon.color: root.selected ? Theme.focusAccent
                              : root.down ? Theme.textPrimary
                              : root.hovered ? Theme.textSecondary
                                             : Theme.textMuted
    activeFocusOnTab: true
    Accessible.name: root.tooltipText
    scale: root.down ? 0.96 : 1

    HoverHandler {
        id: pointerHover

        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
    }

    ToolTip.delay: 600
    ToolTip.visible: pointerHover.hovered
    ToolTip.text: root.tooltipText

    Behavior on scale {
        NumberAnimation { duration: 70 }
    }

    background: Shape {
        id: frame

        readonly property color fillColor: root.down ? Theme.surfaceRaised
                                                     : root.selected ? Theme.surfaceElevated
                                                                     : root.hovered ? Theme.surfaceElevated
                                                                                    : "transparent"
        readonly property color frameColor: root.activeFocus ? Theme.focusAccent
                                                             : root.selected ? Theme.primaryAccent
                                                                             : "transparent"

        ShapePath {
            fillColor: frame.fillColor
            strokeColor: frame.frameColor
            strokeWidth: root.activeFocus ? 2 : root.selected ? 1 : 0
            joinStyle: ShapePath.MiterJoin
            startX: 0
            startY: 0
            PathLine { x: frame.width - Theme.navigationFrameChamfer; y: 0 }
            PathLine { x: frame.width; y: Theme.navigationFrameChamfer }
            PathLine { x: frame.width; y: frame.height }
            PathLine { x: Theme.navigationFrameChamfer; y: frame.height }
            PathLine { x: 0; y: frame.height - Theme.navigationFrameChamfer }
            PathLine { x: 0; y: 0 }
        }
    }
}
