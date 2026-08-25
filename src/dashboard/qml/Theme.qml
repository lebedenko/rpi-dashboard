import QtQuick
pragma Singleton

QtObject {
    readonly property var installedFontFamilies: Qt.fontFamilies()
    readonly property string sansFontFamily: installedFontFamilies.includes("Rajdhani") ? "Rajdhani" : "sans-serif"
    readonly property string fixedFontFamily: preferredFixedFontFamily()
    readonly property int headingFontWeight: Font.DemiBold
    readonly property int informationFontWeight: Font.Medium
    readonly property int metricFontWeight: Font.Light
    readonly property int technicalFontWeight: Font.Medium
    readonly property color background: "#0C1118"
    readonly property color surface: "#131A24"
    readonly property color surfaceElevated: "#18212D"
    readonly property color surfaceRaised: "#202B39"
    readonly property color textPrimary: "#E7EDF5"
    readonly property color textSecondary: "#C5D0DE"
    readonly property color textMuted: "#8D99AD"
    readonly property color primaryAccent: "#5EA2FF"
    readonly property color focusAccent: "#56D7FF"
    readonly property color passiveBorder: "#36465A"
    readonly property color onlineStatus: "#69E6B1"
    readonly property color cpuSeries: "#35A7FF"
    readonly property color memorySeries: "#A875F5"
    readonly property color metricRail: "#1A2A38"
    readonly property color chartText: "#71859B"
    readonly property color chartGrid: "#1A71859B"
    readonly property color chartAxis: "#3371859B"
    readonly property color selectedSurface: "#142431"
    readonly property color cardSurface: "#0E1823"
    readonly property color cardFrame: "#2D6685"
    readonly property color cardAccent: "#20BFFF"
    readonly property color sectionDivider: "#20384B"
    readonly property color sectionDividerStrong: "#2B5872"
    readonly property color badgeSurface: "#0B1D22"
    readonly property color onlineFrame: "#1D5B53"
    readonly property color detailRailSurface: "#0B2230"
    readonly property color detailRailFrame: "#185577"
    readonly property color selectedActionSurface: "#12698B"
    readonly property color selectedActionFrame: "#23C8FF"
    readonly property color selectedActionContent: "#BCEFFF"
    readonly property color disabledActionSurface: "#101B26"
    readonly property color disabledActionFrame: "#385167"
    readonly property color disabledActionContent: "#7F93A8"
    readonly property int spacingSmall: 8
    readonly property int spacingMedium: 16
    readonly property int spacingLarge: 24
    readonly property int touchTarget: 48
    readonly property int displaySafeInset: 10
    readonly property int sidebarWidth: 64
    readonly property int sidebarChamfer: 12
    readonly property int sidebarCornerRadius: 4
    readonly property int navigationFrameChamfer: 8
    readonly property int navigationFrameCornerRadius: 2
    readonly property int deviceHeaderHeight: 64
    readonly property int deviceFrameChamfer: 12
    readonly property int deviceFrameStep: 24
    readonly property int deviceCardGap: 8
    readonly property int nextCardPeek: 24
    readonly property real cardFrameInset: 1
    readonly property real detailsWidthRatio: 0.32
    readonly property int statusBadgeWidth: 132
    readonly property int statusBadgeHeight: 36
    readonly property int badgeChamfer: 12
    readonly property int sectionGap: 24
    readonly property int detailIconRailWidth: 34
    readonly property int detailHeaderHeight: 20
    readonly property int plotLeftPadding: 32
    readonly property int plotRightPadding: 8
    readonly property int plotTopPadding: 30
    readonly property int plotBottomPadding: 18
    readonly property int deviceFooterHeight: 48
    readonly property int footerGap: 8
    readonly property int actionChamfer: 8
    readonly property int headingTextSize: 22
    readonly property int metricTextSize: 18
    readonly property int metricLabelTextSize: 12
    readonly property int bodyTextSize: 14
    readonly property int sectionTitleTextSize: 12
    readonly property int actionTextSize: 13
    readonly property int captionTextSize: 10
    readonly property int axisTextSize: 9

    function preferredFixedFontFamily() : string {
        if (installedFontFamilies.includes("JetBrains Mono"))
            return "JetBrains Mono";

        if (installedFontFamilies.includes("IBM Plex Mono"))
            return "IBM Plex Mono";

        return "monospace";
    }

}
