import QtQuick
import QtQuick.Shapes

Item {
    id: root

    enum LineStyle {
        Solid,
        Dotted
    }

    property int lineWidth: 1
    property int orientation: Qt.Horizontal
    property int lineStyle: Separator.Solid
    property color color: Theme.passiveBorder

    readonly property real devicePixelRatio: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1
    readonly property real logicalLineWidth: Math.max(0, root.lineWidth) / root.devicePixelRatio

    implicitWidth: root.orientation === Qt.Vertical ? root.logicalLineWidth : 24
    implicitHeight: root.orientation === Qt.Horizontal ? root.logicalLineWidth : 24
    opacity: 0.5
    Accessible.ignored: true

    function alignedCenter(): real {
        const vertical = root.orientation === Qt.Vertical;
        const sceneOriginPoint = root.mapToItem(null, 0, 0);
        const sceneOrigin = vertical ? sceneOriginPoint.x : sceneOriginPoint.y;
        const extent = vertical ? root.width : root.height;
        const physicalCenter = (sceneOrigin + extent / 2) * root.devicePixelRatio;
        const alignedPhysicalCenter = Math.round(physicalCenter - root.lineWidth / 2) + root.lineWidth / 2;
        return alignedPhysicalCenter / root.devicePixelRatio - sceneOrigin;
    }

    property real alignedStrokeCenter: 0

    function updateAlignment(): void {
        Qt.callLater(function () {
            root.alignedStrokeCenter = root.alignedCenter();
        });
    }

    onXChanged: root.updateAlignment()
    onYChanged: root.updateAlignment()
    onWidthChanged: root.updateAlignment()
    onHeightChanged: root.updateAlignment()
    onLineWidthChanged: root.updateAlignment()
    onOrientationChanged: root.updateAlignment()
    onDevicePixelRatioChanged: root.updateAlignment()
    Component.onCompleted: root.updateAlignment()

    Connections {
        target: root.parent

        function onXChanged(): void {
            root.updateAlignment();
        }
        function onYChanged(): void {
            root.updateAlignment();
        }
        function onWidthChanged(): void {
            root.updateAlignment();
        }
        function onHeightChanged(): void {
            root.updateAlignment();
        }
    }

    Shape {
        objectName: "separatorShape"
        anchors.fill: parent
        asynchronous: true
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            id: path
            objectName: "separatorPath"

            readonly property bool vertical: root.orientation === Qt.Vertical
            readonly property real center: root.alignedStrokeCenter

            strokeColor: root.color
            strokeWidth: root.logicalLineWidth
            strokeStyle: root.lineStyle === Separator.Dotted ? ShapePath.DashLine : ShapePath.SolidLine
            dashPattern: [1, 3]
            capStyle: ShapePath.RoundCap
            fillColor: "transparent"
            startX: path.vertical ? path.center : 0
            startY: path.vertical ? 0 : path.center

            PathLine {
                x: path.vertical ? path.center : root.width
                y: path.vertical ? root.height : path.center
            }
        }
    }
}
