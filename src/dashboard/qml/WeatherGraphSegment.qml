import QtQuick
import QtQuick.Shapes

Item {
    id: root

    property real position: 0.5
    property real previousPosition: 0.5
    property string segmentObjectName: ""
    property string shapeObjectName: ""
    property bool segmentVisible: true

    objectName: root.segmentObjectName

    Shape {
        objectName: root.shapeObjectName
        x: -root.width / 2
        width: root.width
        height: root.height
        visible: root.segmentVisible
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: Theme.primaryAccent
            strokeWidth: 1
            capStyle: ShapePath.RoundCap
            fillColor: "transparent"
            startX: 0
            startY: (1 - root.previousPosition) * root.height

            PathLine {
                x: root.width
                y: (1 - root.position) * root.height
            }
        }
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        y: (1 - root.position) * root.height - height / 2
        width: 5
        height: 5
        radius: width / 2
        color: Theme.primaryAccent
        antialiasing: true
    }
}
