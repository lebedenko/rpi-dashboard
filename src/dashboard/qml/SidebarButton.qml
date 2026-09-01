import QtQuick
import QtQuick.Controls.Basic
import Rpi.Dashboard as Dashboard

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
        color: root.selected ? Theme.focusAccent : root.down ? Theme.textPrimary : root.hovered ? Theme.textSecondary : Theme.textMuted
    }

    HoverHandler {
        id: pointerHover

        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
    }

    background: Dashboard.Frame {
        backgroundColor: root.down ? Theme.surfaceRaised : root.selected ? Theme.selectedSurface : root.hovered ? Theme.surfaceElevated : "transparent"
        color: root.activeFocus ? Theme.focusAccent : root.selected ? Theme.primaryAccent : "transparent"
        lineWidth: root.activeFocus ? 2 : root.selected ? 1 : 0
        corners: ({
                "topLeft": {
                    "chamfered": Theme.chamferMedium
                },
                "topRight": {
                    "rounded": Theme.radiusSmall
                },
                "bottomRight": {
                    "chamfered": Theme.chamferMedium
                },
                "bottomLeft": {
                    "rounded": Theme.radiusSmall
                }
            })
    }

    Behavior on scale {
        NumberAnimation {
            duration: 70
        }
    }
}
