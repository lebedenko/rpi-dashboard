import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Button {
    id: root

    property bool selected: false
    property url iconSource
    property string tooltipText

    display: AbstractButton.IconOnly
    activeFocusOnTab: true
    scale: root.down ? 0.96 : 1
    Accessible.name: root.tooltipText
    ToolTip {
        delay: 600
        visible: pointerHover.hovered
        text: root.tooltipText
    }
    icon {
        source: root.iconSource
        width: 24
        height: 24
        color: root.selected ? Theme.focusAccent
                             : root.down ? Theme.textPrimary
                             : root.hovered ? Theme.textSecondary
                                            : Theme.textMuted
    }
    background: Shape {
        ShapePath {
            fillColor: root.down ? Theme.surfaceRaised
                                 : root.selected ? Theme.selectedSurface
                                                 : root.hovered ? Theme.surfaceElevated
                                                                : "transparent"
            strokeColor: root.activeFocus ? Theme.focusAccent
                                          : root.selected ? Theme.primaryAccent
                                                          : "transparent"
            strokeWidth: root.activeFocus ? 2 : root.selected ? 1 : 0
            joinStyle: ShapePath.MiterJoin
            startX: Theme.navigationFrameChamfer
            startY: 0
            PathLine { x: root.width - Theme.navigationFrameCornerRadius; y: 0 }
            PathArc {
                x: root.width
                y: Theme.navigationFrameCornerRadius
                radiusX: Theme.navigationFrameCornerRadius
                radiusY: Theme.navigationFrameCornerRadius
            }
            PathLine { x: root.width; y: root.height - Theme.navigationFrameChamfer }
            PathLine { x: root.width - Theme.navigationFrameChamfer; y: root.height }
            PathLine { x: Theme.navigationFrameCornerRadius; y: root.height }
            PathArc {
                x: 0
                y: root.height - Theme.navigationFrameCornerRadius
                radiusX: Theme.navigationFrameCornerRadius
                radiusY: Theme.navigationFrameCornerRadius
            }
            PathLine { x: 0; y: Theme.navigationFrameChamfer }
            PathLine { x: Theme.navigationFrameChamfer; y: 0 }
        }
    }

    HoverHandler {
        id: pointerHover

        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
    }

    Behavior on scale {
        NumberAnimation { duration: 70 }
    }
}
