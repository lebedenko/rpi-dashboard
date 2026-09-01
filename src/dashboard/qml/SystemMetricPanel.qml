import QtQuick
import QtQuick.Controls.Basic
import Rpi.Dashboard as Dashboard

Control {
    id: root

    required property string heading
    required property string primaryValue
    required property string secondaryLabel
    required property string secondaryValue
    property real usageRatio: -1
    property color accentColor: Theme.primaryAccent
    property string accessibleSummary: qsTr("%1, %2, %3 %4")
                                       .arg(root.heading).arg(root.primaryValue)
                                       .arg(root.secondaryLabel).arg(root.secondaryValue)

    padding: Theme.spacingSmall

    Accessible.role: Accessible.StaticText
    Accessible.name: root.accessibleSummary

    background: Dashboard.Frame {
        backgroundColor: Theme.cardSurface
        lineWidth: 1
        color: Theme.cardFrame
        corners: ({ rounded: Theme.radiusMedium })
    }

    contentItem: Item {
        Label {
            id: headingLabel
            anchors.left: parent.left
            anchors.top: parent.top
            text: root.heading
            color: root.accentColor
            font.family: Theme.fixedFontFamily
            font.pixelSize: Theme.sectionTitleTextSize
            font.weight: Theme.technicalFontWeight
        }

        Label {
            id: primaryLabel
            objectName: root.objectName + "Primary"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: headingLabel.bottom
            anchors.topMargin: Theme.spacingMedium
            text: root.primaryValue
            color: Theme.textPrimary
            font.family: Theme.sansFontFamily
            font.pixelSize: 26
            font.weight: Theme.headingFontWeight
            elide: Text.ElideRight
        }

        Rectangle {
            id: rail
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: primaryLabel.bottom
            anchors.topMargin: Theme.spacingMedium
            height: 6
            color: Theme.metricRail
            Accessible.ignored: true

            Rectangle {
                objectName: root.objectName + "Gauge"
                width: root.usageRatio >= 0 ? parent.width * Math.min(1, root.usageRatio) : 0
                height: parent.height
                visible: root.usageRatio >= 0
                color: root.accentColor
                Accessible.ignored: true
            }
        }

        Label {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            text: root.secondaryLabel
            color: Theme.textMuted
            font.family: Theme.fixedFontFamily
            font.pixelSize: Theme.captionTextSize
        }

        Label {
            objectName: root.objectName + "Secondary"
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: 54
            text: root.secondaryValue
            color: Theme.textSecondary
            horizontalAlignment: Text.AlignRight
            font.family: Theme.fixedFontFamily
            font.pixelSize: Theme.bodyTextSize
            elide: Text.ElideLeft
        }
    }
}
