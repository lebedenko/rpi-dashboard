import Rpi.Dashboard
import QtQuick
import QtTest

Item {
    id: root
    width: 160
    height: 120

    Component {
        id: frameComponent

        Frame {
            width: 40
            height: 40
        }
    }

    TestCase {
        name: "FrameTests"
        when: windowShown

        function createFrame(properties = {}) {
            const frame = createTemporaryObject(frameComponent, root, properties);
            verify(!!frame, "Component exists");
            return frame;
        }

        function renderedImage(frame) {
            waitForRendering(frame);
            return grabImage(frame);
        }

        function test_defaults() {
            const frame = createFrame();
            compare(frame.color, Theme.passiveBorder);
            compare(frame.lineWidth, 1);
            compare(frame.backgroundColor, "#00000000");
            compare(frame.corners, {});
        }

        function test_uniformRoundedCorners() {
            const frame = createFrame({
                "backgroundColor": "#ff0000",
                "color": "#00000000",
                "lineWidth": 0,
                "corners": ({ "rounded": 12 })
            });
            const image = renderedImage(frame);
            compare(image.pixel(1, 1), "#ffffff");
            compare(image.pixel(3, 7), "#ff0000");
            compare(image.pixel(20, 20), "#ff0000");
        }

        function test_uniformChamferedCorners() {
            const frame = createFrame({
                "backgroundColor": "#00ff00",
                "color": "#00000000",
                "lineWidth": 0,
                "corners": ({ "chamfered": 12 })
            });
            const image = renderedImage(frame);
            compare(image.pixel(3, 7), "#ffffff");
            compare(image.pixel(20, 20), "#00ff00");
        }

        function test_mixedCornersAndSquareFallback() {
            const frame = createFrame({
                "backgroundColor": "#0000ff",
                "color": "#00000000",
                "lineWidth": 0,
                "corners": ({
                    "topLeft": { "rounded": 10 },
                    "bottomRight": { "chamfered": 10 }
                })
            });
            const image = renderedImage(frame);
            compare(image.pixel(1, 1), "#ffffff");
            compare(image.pixel(38, 1), "#0000ff");
            compare(image.pixel(38, 38), "#ffffff");
            compare(image.pixel(1, 38), "#0000ff");
        }

        function test_customBorderProperties() {
            const frame = createFrame();
            frame.color = "#123456";
            frame.lineWidth = 4;
            compare(frame.color, "#123456");
            compare(frame.lineWidth, 4);
        }

        function test_invalidAndOversizedCornersRemainValid() {
            const fillProperties = {
                "backgroundColor": "#ff00ff",
                "color": "#00000000",
                "lineWidth": 0
            };
            const negative = createFrame(Object.assign({}, fillProperties,
                                                        { "corners": ({ "rounded": -4 }) }));
            compare(renderedImage(negative).pixel(1, 1), "#ff00ff");
            negative.destroy();
            const malformed = createFrame(Object.assign({}, fillProperties,
                                                         { "corners": ({ "topLeft": { "rounded": "large" } }) }));
            compare(renderedImage(malformed).pixel(1, 1), "#ff00ff");
            malformed.destroy();
            const nonFinite = createFrame(Object.assign({}, fillProperties,
                                                         { "corners": ({ "chamfered": Number.POSITIVE_INFINITY }) }));
            compare(renderedImage(nonFinite).pixel(1, 1), "#ff00ff");
            nonFinite.destroy();
            const oversized = createFrame(Object.assign({}, fillProperties,
                                                         { "corners": ({ "rounded": 100 }) }));
            compare(renderedImage(oversized).pixel(1, 1), "#ffffff");
            oversized.width = 80;
            oversized.height = 20;
            compare(renderedImage(oversized).pixel(1, 1), "#ffffff");
        }

        function test_reassignedConfigurationUpdatesGeometry() {
            const frame = createFrame({ "corners": ({ "rounded": 4 }) });
            frame.corners = ({ "topLeft": { "chamfered": 8 } });
            compare(frame.corners.topLeft.chamfered, 8);
            compare(frame.corners.topRight, undefined);
        }
    }
}
