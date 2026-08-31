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
            startX: Theme.chamferMedium
            startY: 0
            PathLine { x: root.width - Theme.radiusSmall; y: 0 }
            PathArc {
                x: root.width
                y: Theme.radiusSmall
                radiusX: Theme.radiusSmall
                radiusY: Theme.radiusSmall
            }
            PathLine { x: root.width; y: root.height - Theme.chamferMedium }
            PathLine { x: root.width - Theme.chamferMedium; y: root.height }
            PathLine { x: Theme.radiusSmall; y: root.height }
            PathArc {
                x: 0
                y: root.height - Theme.radiusSmall
                radiusX: Theme.radiusSmall
                radiusY: Theme.radiusSmall
            }
            PathLine { x: 0; y: Theme.chamferMedium }
            PathLine { x: Theme.chamferMedium; y: 0 }
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
