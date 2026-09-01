import QtQuick
import QtQuick.Shapes
import QtQuick.Window
import QtTest
import Rpi.Dashboard

Item {
    id: root

    width: 1500
    height: 340

    Window {
        id: testWindow

        width: root.width
        height: root.height
        visible: true
    }

    Component {
        id: separatorComponent

        Separator {}
    }

    Component {
        id: weatherPageComponent

        WeatherPage {}
    }

    Component {
        id: clockSidebarComponent

        ClockSidebar {}
    }

    Component {
        id: weatherServiceComponent

        QtObject {
            property string state: "ready"
            property string diagnostics: ""
            property string city: "Lviv"
            property string country: "UA"
            property date lastSuccessUtc: new Date()
            property bool stale: false
            property real temperatureCelsius: 20
            property real feelsLikeCelsius: 20
            property real highCelsius: 24
            property real lowCelsius: 14
            property real humidityPercent: 50
            property real windSpeedKmh: 8
            property string windDirection: "N"
            property string condition: "Clear"
            property string iconCode: "01d"
            property int airQualityIndex: 2
            property string airQualityCategory: "Good"
            property string nextSolarEventKind: "sunset"
            property string localNextSolarEventTime: "20:30"
            property real todayPrecipitationProbabilityPercent: 20
            property string todayPrecipitationKind: "rain"
            property ListModel hourlyModel
            property ListModel dailyModel

            function refresh() {
            }

            hourlyModel: ListModel {
                ListElement {
                    localHour: "12"
                    iconCode: "01d"
                    temperatureCelsius: 20
                    precipitationProbabilityPercent: 0
                    trendPosition: 0.5
                    previousTrendPosition: 0.5
                }

                ListElement {
                    localHour: "13"
                    iconCode: "01d"
                    temperatureCelsius: 21
                    precipitationProbabilityPercent: 0
                    trendPosition: 0.4
                    previousTrendPosition: 0.5
                }

                ListElement {
                    localHour: "14"
                    iconCode: "01d"
                    temperatureCelsius: 22
                    precipitationProbabilityPercent: 0
                    trendPosition: 0.3
                    previousTrendPosition: 0.4
                }

                ListElement {
                    localHour: "15"
                    iconCode: "01d"
                    temperatureCelsius: 23
                    precipitationProbabilityPercent: 0
                    trendPosition: 0.2
                    previousTrendPosition: 0.3
                }

                ListElement {
                    localHour: "16"
                    iconCode: "01d"
                    temperatureCelsius: 22
                    precipitationProbabilityPercent: 0
                    trendPosition: 0.3
                    previousTrendPosition: 0.2
                }

                ListElement {
                    localHour: "17"
                    iconCode: "01d"
                    temperatureCelsius: 21
                    precipitationProbabilityPercent: 0
                    trendPosition: 0.4
                    previousTrendPosition: 0.3
                }

                ListElement {
                    localHour: "18"
                    iconCode: "01d"
                    temperatureCelsius: 20
                    precipitationProbabilityPercent: 0
                    trendPosition: 0.5
                    previousTrendPosition: 0.4
                }

                ListElement {
                    localHour: "19"
                    iconCode: "01d"
                    temperatureCelsius: 19
                    precipitationProbabilityPercent: 0
                    trendPosition: 0.6
                    previousTrendPosition: 0.5
                }
            }

            dailyModel: ListModel {
                ListElement {
                    weekday: "Mon"
                    iconCode: "01d"
                    minimumCelsius: 10
                    maximumCelsius: 20
                    averageCelsius: 15
                    precipitationProbabilityPercent: 0
                }

                ListElement {
                    weekday: "Tue"
                    iconCode: "01d"
                    minimumCelsius: 10
                    maximumCelsius: 20
                    averageCelsius: 15
                    precipitationProbabilityPercent: 0
                }

                ListElement {
                    weekday: "Wed"
                    iconCode: "01d"
                    minimumCelsius: 10
                    maximumCelsius: 20
                    averageCelsius: 15
                    precipitationProbabilityPercent: 0
                }

                ListElement {
                    weekday: "Thu"
                    iconCode: "01d"
                    minimumCelsius: 10
                    maximumCelsius: 20
                    averageCelsius: 15
                    precipitationProbabilityPercent: 0
                }

                ListElement {
                    weekday: "Fri"
                    iconCode: "01d"
                    minimumCelsius: 10
                    maximumCelsius: 20
                    averageCelsius: 15
                    precipitationProbabilityPercent: 0
                }
            }
        }
    }

    TestCase {
        function createSeparator(properties = {}) {
            const separator = separatorComponent.createObject(testWindow.contentItem, properties);
            verify(separator !== null);
            return separator;
        }

        function isIntegral(value) {
            return Math.abs(value - Math.round(value)) < 0.0001;
        }

        function verifySeparator(separator, orientation, lineStyle, expectedLength) {
            verify(separator !== null);
            wait(0);
            compare(separator.orientation, orientation);
            compare(separator.lineStyle, lineStyle);
            compare(separator.color, Theme.sectionDividerStrong);
            compare(separator.lineWidth, 1);
            compare(separator.opacity, 0.5);
            const path = findChild(separator, "separatorPath");
            const dpr = separator.devicePixelRatio;
            const origin = separator.mapToItem(testWindow.contentItem, 0, 0);
            const sceneCenter = (orientation === Qt.Vertical ? origin.x : origin.y) + path.center;
            compare(path.strokeWidth * dpr, 1);
            const lowerBoundary = (sceneCenter - path.strokeWidth / 2) * dpr;
            const upperBoundary = (sceneCenter + path.strokeWidth / 2) * dpr;
            verify(isIntegral(lowerBoundary), separator.objectName + " lower boundary " + lowerBoundary);
            verify(isIntegral(upperBoundary), separator.objectName + " upper boundary " + upperBoundary);
            compare(orientation === Qt.Vertical ? separator.height : separator.width, expectedLength);
        }

        function test_defaults() {
            const separator = createSeparator();
            compare(separator.lineWidth, 1);
            compare(separator.orientation, Qt.Horizontal);
            compare(separator.lineStyle, Separator.Solid);
            compare(separator.color, Theme.passiveBorder);
            compare(separator.opacity, 0.5);
            compare(separator.Accessible.ignored, true);
            compare(findChild(separator, "separatorShape").preferredRendererType, Shape.CurveRenderer);
            separator.destroy();
        }

        function test_orientationStyleAndColor() {
            const separator = createSeparator({
                "width": 40,
                "height": 30,
                "orientation": Qt.Vertical,
                "lineStyle": Separator.Dotted,
                "color": "#123456",
                "opacity": 0.75
            });
            const path = findChild(separator, "separatorPath");
            compare(path.strokeStyle, ShapePath.DashLine);
            compare(path.dashPattern, [1, 3]);
            compare(path.capStyle, ShapePath.RoundCap);
            compare(path.strokeColor, "#123456");
            compare(separator.opacity, 0.75);
            compare(path.startX, path.center);
            compare(path.startY, 0);
            separator.orientation = Qt.Horizontal;
            separator.lineStyle = Separator.Solid;
            compare(path.strokeStyle, ShapePath.SolidLine);
            compare(path.startX, 0);
            compare(path.startY, path.center);
            separator.destroy();
        }

        function test_physicalPixelWidthAndAlignment() {
            const separator = createSeparator({
                "x": 0.25,
                "y": 0.5,
                "width": 101,
                "height": 17,
                "lineWidth": 1
            });
            wait(0);
            const path = findChild(separator, "separatorPath");
            const dpr = separator.devicePixelRatio;
            let origin = separator.mapToItem(testWindow.contentItem, 0, 0);
            compare(path.strokeWidth * dpr, 1);
            verify(isIntegral((origin.y + path.center - path.strokeWidth / 2) * dpr));
            verify(isIntegral((origin.y + path.center + path.strokeWidth / 2) * dpr));
            separator.lineWidth = 2;
            wait(0);
            compare(path.strokeWidth * dpr, 2);
            verify(isIntegral((origin.y + path.center - path.strokeWidth / 2) * dpr));
            verify(isIntegral((origin.y + path.center + path.strokeWidth / 2) * dpr));
            separator.orientation = Qt.Vertical;
            wait(0);
            origin = separator.mapToItem(testWindow.contentItem, 0, 0);
            verify(isIntegral((origin.x + path.center - path.strokeWidth / 2) * dpr));
            verify(isIntegral((origin.x + path.center + path.strokeWidth / 2) * dpr));
            separator.destroy();
        }

        function test_weatherExperienceSeparators() {
            const service = weatherServiceComponent.createObject(testWindow.contentItem);
            const page = weatherPageComponent.createObject(testWindow.contentItem, {
                "x": 0.25,
                "y": 0.5,
                "width": 1480,
                "height": 320,
                "service": service
            });
            const sidebar = clockSidebarComponent.createObject(testWindow.contentItem, {
                "x": 0.5,
                "y": 0.25,
                "width": 144,
                "height": 320,
                "pageContext": "weather",
                "weatherService": service
            });
            verify(page !== null);
            verify(sidebar !== null);
            wait(0);
            const currentContent = findChild(page, "currentContent");
            verifySeparator(findChild(page, "currentSeparator"), Qt.Horizontal, Separator.Solid, currentContent.width);
            const currentMetrics = findChild(page, "currentMetrics");
            for (let index = 1; index < 4; ++index)
                verifySeparator(findChild(page, "currentMetricSeparator" + index), Qt.Vertical, Separator.Dotted, currentMetrics.height);
            const hourlyPanel = findChild(page, "hourlyForecastPanel");
            for (let index = 1; index < 8; ++index) {
                const separator = findChild(page, "hourlySeparator" + index);
                verifySeparator(separator, Qt.Vertical, Separator.Dotted, separator.parent.height);
            }
            const dateSeparator = findChild(sidebar, "clockDateSeparator");
            verifySeparator(dateSeparator, Qt.Horizontal, Separator.Dotted, dateSeparator.parent.width - Theme.spacingMedium);
            const weatherRail = findChild(sidebar, "weatherRail");
            verifySeparator(findChild(sidebar, "weatherRailSeparator1"), Qt.Horizontal, Separator.Dotted, weatherRail.width);
            verifySeparator(findChild(sidebar, "weatherRailSeparator2"), Qt.Horizontal, Separator.Dotted, weatherRail.width);
            page.destroy();
            sidebar.destroy();
            service.destroy();
        }

        function test_dailyRowsUseSnappedCumulativeBoundaries() {
            const service = weatherServiceComponent.createObject(testWindow.contentItem);
            const page = weatherPageComponent.createObject(testWindow.contentItem, {
                "width": 1480,
                "height": 320,
                "service": service
            });
            verify(page !== null);
            wait(0);
            const panel = findChild(page, "dailyForecastPanel");
            const dpr = panel.Screen.devicePixelRatio;
            const heights = [];
            let totalHeight = 0;
            let firstRow = null;
            let previousRow = null;
            for (let index = 0; index < 5; ++index) {
                const row = findChild(page, "dailyRow" + index).parent;
                if (firstRow === null)
                    firstRow = row;

                if (previousRow !== null)
                    compare(row.y, previousRow.y + previousRow.height);

                previousRow = row;
                heights.push(row.height * dpr);
                totalHeight += row.height;
                verify(isIntegral(row.y * dpr));
                verify(isIntegral((row.y + row.height) * dpr));
                if (index > 0) {
                    const separator = findChild(page, "dailySeparator" + index);
                    compare(separator.anchors.leftMargin, 8);
                    compare(separator.anchors.rightMargin, 8);
                    compare(separator.width, row.width - 16);
                    compare(separator.orientation, Qt.Horizontal);
                    compare(separator.lineStyle, Separator.Solid);
                    compare(separator.lineWidth, 1);
                    compare(separator.color, Theme.sectionDividerStrong);
                    compare(separator.opacity, 0.5);
                    const path = findChild(separator, "separatorPath");
                    const origin = separator.mapToItem(testWindow.contentItem, 0, 0);
                    verify(isIntegral((origin.y + path.center - path.strokeWidth / 2) * dpr));
                    verify(isIntegral((origin.y + path.center + path.strokeWidth / 2) * dpr));
                }
            }
            compare(totalHeight, previousRow.y + previousRow.height - firstRow.y);
            verify(Math.abs(panel.height - (previousRow.y + previousRow.height)) * dpr < 1);
            verify(Math.max(...heights) - Math.min(...heights) <= 1.0001, "row heights: " + heights.join(", "));
            page.destroy();
            service.destroy();
        }

        name: "Separator"
        when: windowShown
    }
}
