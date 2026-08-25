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
    readonly property color background: "#020A13"
    readonly property color surface: "#061321"
    readonly property color surfaceElevated: "#0A1A2B"
    readonly property color surfaceRaised: "#10283C"
    readonly property color textPrimary: "#F2F7FC"
    readonly property color textSecondary: "#B7C7D9"
    readonly property color textMuted: "#8295AC"
    readonly property color primaryAccent: "#19D3F3"
    readonly property color focusAccent: "#5DE7FF"
    readonly property color passiveBorder: "#3B6888"
    readonly property color onlineStatus: "#50F0A0"
    readonly property color cpuSeries: "#36B9FF"
    readonly property color memorySeries: "#A66CFF"
    readonly property color metricRail: "#123149"
    readonly property color chartText: "#8295AC"
    readonly property color chartGrid: "#1A8295AC"
    readonly property color chartAxis: "#338295AC"
    readonly property color selectedSurface: "#09283C"
    readonly property color cardSurface: "#041321"
    readonly property color cardFrame: "#28779C"
    readonly property color cardAccent: "#20D4F7"
    readonly property color sectionDivider: "#17374D"
    readonly property color sectionDividerStrong: "#2B6685"
    readonly property color badgeSurface: "#06251F"
    readonly property color onlineFrame: "#258667"
    readonly property color detailRailSurface: "#08283A"
    readonly property color detailRailFrame: "#21789B"
    readonly property color selectedActionSurface: "#0D6587"
    readonly property color selectedActionFrame: "#20D4F7"
    readonly property color selectedActionContent: "#D5F7FF"
    readonly property color disabledActionSurface: "#102236"
    readonly property color disabledActionFrame: "#3D617E"
    readonly property color disabledActionContent: "#91A5BB"
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
