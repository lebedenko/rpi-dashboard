import QtQuick
import QtQuick.Shapes

Item {
    id: frame

    property color color: Theme.passiveBorder
    property real lineWidth: 1
    property color backgroundColor: "transparent"
    property var corners: ({})

    Accessible.ignored: true

    Shape {
        anchors.fill: parent

        ShapePath {
            id: path

            readonly property real maximumCornerSize: Math.max(0, Math.min(frame.width, frame.height) / 2)
            readonly property int topLeftType: path.cornerType("topLeft")
            readonly property int topRightType: path.cornerType("topRight")
            readonly property int bottomRightType: path.cornerType("bottomRight")
            readonly property int bottomLeftType: path.cornerType("bottomLeft")
            readonly property real topLeftSize: path.cornerSize("topLeft")
            readonly property real topRightSize: path.cornerSize("topRight")
            readonly property real bottomRightSize: path.cornerSize("bottomRight")
            readonly property real bottomLeftSize: path.cornerSize("bottomLeft")

            strokeColor: frame.color
            strokeWidth: frame.lineWidth
            fillColor: frame.backgroundColor
            joinStyle: ShapePath.MiterJoin
            startX: path.topLeftSize
            startY: 0

            function cornerDescriptor(cornerName: string): var {
                if (frame.corners === null || typeof frame.corners !== "object")
                    return null;

                const descriptor = frame.corners[cornerName];
                return descriptor === undefined ? frame.corners : descriptor;
            }

            function cornerType(cornerName: string): int {
                const descriptor = path.cornerDescriptor(cornerName);
                if (descriptor === null || typeof descriptor !== "object")
                    return 0;
                if (typeof descriptor.rounded === "number")
                    return 1;
                if (typeof descriptor.chamfered === "number")
                    return 2;
                return 0;
            }

            function cornerSize(cornerName: string): real {
                const descriptor = path.cornerDescriptor(cornerName);
                if (descriptor === null || typeof descriptor !== "object")
                    return 0;

                const type = path.cornerType(cornerName);
                const requestedSize = type === 1 ? descriptor.rounded : type === 2 ? descriptor.chamfered : 0;
                if (!Number.isFinite(requestedSize) || requestedSize <= 0)
                    return 0;
                return Math.min(requestedSize, path.maximumCornerSize);
            }

            PathLine {
                x: frame.width - path.topRightSize
                y: 0
            }
            PathArc {
                x: frame.width
                y: path.topRightSize
                radiusX: path.topRightType === 1 ? path.topRightSize : 0
                radiusY: radiusX
                direction: PathArc.Clockwise
            }
            PathLine {
                x: frame.width
                y: frame.height - path.bottomRightSize
            }
            PathArc {
                x: frame.width - path.bottomRightSize
                y: frame.height
                radiusX: path.bottomRightType === 1 ? path.bottomRightSize : 0
                radiusY: radiusX
                direction: PathArc.Clockwise
            }
            PathLine {
                x: path.bottomLeftSize
                y: frame.height
            }
            PathArc {
                x: 0
                y: frame.height - path.bottomLeftSize
                radiusX: path.bottomLeftType === 1 ? path.bottomLeftSize : 0
                radiusY: radiusX
                direction: PathArc.Clockwise
            }
            PathLine {
                x: 0
                y: path.topLeftSize
            }
            PathArc {
                x: path.topLeftSize
                y: 0
                radiusX: path.topLeftType === 1 ? path.topLeftSize : 0
                radiusY: radiusX
                direction: PathArc.Clockwise
            }
        }
    }
}
