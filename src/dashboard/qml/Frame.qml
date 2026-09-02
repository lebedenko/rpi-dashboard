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
        asynchronous: true
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            id: path

            readonly property real effectiveStrokeWidth: Number.isFinite(frame.lineWidth) && frame.lineWidth > 0 ? frame.lineWidth : 0
            readonly property real inset: path.effectiveStrokeWidth / 2
            readonly property real left: path.inset
            readonly property real top: path.inset
            readonly property real right: Math.max(path.left, frame.width - path.inset)
            readonly property real bottom: Math.max(path.top, frame.height - path.inset)
            readonly property real maximumCornerSize: Math.max(0, Math.min(path.right - path.left, path.bottom - path.top) / 2)
            readonly property int topLeftType: path.cornerType("topLeft")
            readonly property int topRightType: path.cornerType("topRight")
            readonly property int bottomRightType: path.cornerType("bottomRight")
            readonly property int bottomLeftType: path.cornerType("bottomLeft")
            readonly property real topLeftSize: path.cornerSize("topLeft")
            readonly property real topRightSize: path.cornerSize("topRight")
            readonly property real bottomRightSize: path.cornerSize("bottomRight")
            readonly property real bottomLeftSize: path.cornerSize("bottomLeft")
            readonly property string pathData: ["M " + (path.left + path.topLeftSize) + " " + path.top, "L " + (path.right - path.topRightSize) + " " + path.top, path.cornerCommand(path.topRightType, path.topRightSize, path.right, path.top + path.topRightSize), "L " + path.right + " " + (path.bottom - path.bottomRightSize), path.cornerCommand(path.bottomRightType, path.bottomRightSize, path.right - path.bottomRightSize, path.bottom), "L " + (path.left + path.bottomLeftSize) + " " + path.bottom, path.cornerCommand(path.bottomLeftType, path.bottomLeftSize, path.left, path.bottom - path.bottomLeftSize), "L " + path.left + " " + (path.top + path.topLeftSize), path.cornerCommand(path.topLeftType, path.topLeftSize, path.left + path.topLeftSize, path.top), "Z"].join(" ")

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

            function cornerCommand(type: int, size: real, x: real, y: real): string {
                return type === 1 && size > 0 ? "A " + size + " " + size + " 0 0 1 " + x + " " + y : "L " + x + " " + y;
            }

            PathSvg {
                objectName: "frameSvgPath"
                path: path.pathData
            }
        }
    }
}
