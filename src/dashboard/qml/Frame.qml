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
        objectName: "frameShape"
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            id: path

            readonly property real effectiveStrokeWidth: Number.isFinite(frame.lineWidth) && frame.lineWidth > 0
                                                        ? frame.lineWidth : 0
            readonly property real inset: path.effectiveStrokeWidth / 2
            readonly property real left: path.inset
            readonly property real top: path.inset
            readonly property real right: Math.max(path.left, frame.width - path.inset)
            readonly property real bottom: Math.max(path.top, frame.height - path.inset)
            readonly property real maximumCornerSize: Math.max(0, Math.min(path.right - path.left,
                                                                           path.bottom - path.top) / 2)
            readonly property int topLeftType: path.cornerType("topLeft")
            readonly property int topRightType: path.cornerType("topRight")
            readonly property int bottomRightType: path.cornerType("bottomRight")
            readonly property int bottomLeftType: path.cornerType("bottomLeft")
            readonly property real topLeftSize: path.cornerSize("topLeft")
            readonly property real topRightSize: path.cornerSize("topRight")
            readonly property real bottomRightSize: path.cornerSize("bottomRight")
            readonly property real bottomLeftSize: path.cornerSize("bottomLeft")

            strokeColor: frame.color
            strokeWidth: path.effectiveStrokeWidth
            fillColor: frame.backgroundColor
            joinStyle: ShapePath.MiterJoin
            startX: path.left + path.topLeftSize
            startY: path.top

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
                x: path.right - path.topRightSize
                y: path.top
            }
            PathArc {
                x: path.right
                y: path.top + path.topRightSize
                radiusX: path.topRightType === 1 ? path.topRightSize : 0
                radiusY: radiusX
                direction: PathArc.Clockwise
            }
            PathLine {
                x: path.right
                y: path.bottom - path.bottomRightSize
            }
            PathArc {
                x: path.right - path.bottomRightSize
                y: path.bottom
                radiusX: path.bottomRightType === 1 ? path.bottomRightSize : 0
                radiusY: radiusX
                direction: PathArc.Clockwise
            }
            PathLine {
                x: path.left + path.bottomLeftSize
                y: path.bottom
            }
            PathArc {
                x: path.left
                y: path.bottom - path.bottomLeftSize
                radiusX: path.bottomLeftType === 1 ? path.bottomLeftSize : 0
                radiusY: radiusX
                direction: PathArc.Clockwise
            }
            PathLine {
                x: path.left
                y: path.top + path.topLeftSize
            }
            PathArc {
                x: path.left + path.topLeftSize
                y: path.top
                radiusX: path.topLeftType === 1 ? path.topLeftSize : 0
                radiusY: radiusX
                direction: PathArc.Clockwise
            }
        }
    }
}
