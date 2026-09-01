import QtQuick
pragma Singleton

QtObject {
    id: root

    // Font families and weights
    readonly property list<string> installedFontFamilies: Qt.fontFamilies()
    readonly property bool bundledFontsReady: root.rajdhaniLight.status === FontLoader.Ready
                                               && root.rajdhaniRegular.status === FontLoader.Ready
                                               && root.rajdhaniMedium.status === FontLoader.Ready
                                               && root.rajdhaniSemiBold.status === FontLoader.Ready
                                               && root.jetBrainsLight.status === FontLoader.Ready
                                               && root.jetBrainsRegular.status === FontLoader.Ready
                                               && root.jetBrainsMedium.status === FontLoader.Ready
    readonly property string bundledSansFontFamily: root.rajdhaniRegular.status === FontLoader.Ready ? root.rajdhaniRegular.name : ""
    readonly property string bundledFixedFontFamily: root.jetBrainsRegular.status === FontLoader.Ready ? root.jetBrainsRegular.name : ""
    readonly property string sansFontFamily: root.bundledSansFontFamily.length > 0 ? root.bundledSansFontFamily
                                                                                   : root.installedFontFamilies.includes("Rajdhani") ? "Rajdhani" : "sans-serif"
    readonly property string fixedFontFamily: root.bundledFixedFontFamily.length > 0 ? root.bundledFixedFontFamily
                                                                                      : root.preferredFixedFontFamily()
    readonly property int headingFontWeight: Font.DemiBold
    readonly property int labelFontWeight: Font.Normal
    readonly property int informationFontWeight: Font.Medium
    readonly property int metricFontWeight: Font.Light
    readonly property int technicalFontWeight: Font.Medium
    readonly property int technicalRegularFontWeight: Font.Normal
    readonly property int technicalLightFontWeight: Font.Light

    // Palette
    readonly property color background: "#020A13"
    readonly property color surface: "#061321"
    readonly property color surfaceElevated: "#0A1A2B"
    readonly property color surfaceRaised: "#10283C"
    readonly property color textPrimary: "#F2F7FC"
    readonly property color textSecondary: "#B7C7D9"
    readonly property color textMuted: "#8295AC"
    readonly property color primaryAccent: "#19D3F3"
    readonly property color focusAccent: "#5DE7FF"
    readonly property color blueAccent: "#36B9FF"
    readonly property color violetAccent: "#A66CFF"
    readonly property color passiveBorder: "#3B6888"
    readonly property color onlineStatus: "#50F0A0"
    readonly property color healthyStatus: root.onlineStatus
    readonly property color runningStatus: root.primaryAccent
    readonly property color attentionStatus: "#FFC857"
    readonly property color failureStatus: "#FF5D73"
    readonly property color staleStatus: root.violetAccent
    readonly property color unknownStatus: root.textMuted
    readonly property color cpuSeries: root.blueAccent
    readonly property color memorySeries: root.violetAccent
    readonly property color metricRail: "#123149"
    readonly property color chartText: root.textMuted
    readonly property color chartGrid: Qt.rgba(root.textMuted.r, root.textMuted.g, root.textMuted.b, 26 / 255)
    readonly property color chartAxis: Qt.rgba(root.textMuted.r, root.textMuted.g, root.textMuted.b, 51 / 255)
    readonly property color selectedSurface: "#09283C"
    readonly property color cardSurface: "#041321"
    readonly property color cardFrame: "#28779C"
    readonly property color cardAccent: "#20D4F7"
    readonly property color sectionDivider: "#17374D"
    readonly property color sectionDividerStrong: "#2B6685"
    readonly property color badgeSurface: "#06251F"
    readonly property color detailRailSurface: "#08283A"
    readonly property color detailRailFrame: "#21789B"

    // Shape scale
    readonly property int chamferSmall: 6
    readonly property int chamferMedium: 8
    readonly property int chamferLarge: 12
    readonly property int radiusSmall: 2
    readonly property int radiusMedium: 4
    readonly property int radiusLarge: 8

    // Spacing and layout
    readonly property int spacingSmall: 8
    readonly property int spacingMedium: 16
    readonly property int spacingLarge: 24
    readonly property int touchTarget: 48
    readonly property int displaySafeInset: 10
    readonly property int sidebarWidth: 64
    readonly property int statusSidebarWidth: 144
    readonly property int deviceHeaderHeight: 64
    readonly property int deviceFrameStep: 24
    readonly property int deviceCardGap: 8
    readonly property int nextCardPeek: 24
    readonly property real cardFrameInset: 1
    readonly property real detailsWidthRatio: 0.32
    readonly property int statusBadgeWidth: 132
    readonly property int statusBadgeHeight: 36
    readonly property int sectionGap: 24
    readonly property int detailIconRailWidth: 34
    readonly property int detailHeaderHeight: 20
    readonly property int plotLeftPadding: 32
    readonly property int plotRightPadding: 8
    readonly property int plotTopPadding: 30
    readonly property int plotBottomPadding: 18

    // Typography sizes
    readonly property int headingTextSize: 22
    readonly property int secondaryHeadingTextSize: 20
    readonly property int clockTimeTextSize: 48
    readonly property int clockDateTextSize: 14
    readonly property int metricTextSize: 18
    readonly property int metricLabelTextSize: 12
    readonly property int bodyTextSize: 14
    readonly property int sectionTitleTextSize: 12
    readonly property int captionTextSize: 10
    readonly property int axisTextSize: 9

    function preferredFixedFontFamily() : string {
        if (root.installedFontFamilies.includes("JetBrains Mono"))
            return "JetBrains Mono";

        if (root.installedFontFamilies.includes("IBM Plex Mono"))
            return "IBM Plex Mono";

        return "monospace";
    }

    function statusColor(status: string): color {
        switch (status) {
        case "online":
        case "healthy":
            return root.onlineStatus
        case "registered":
        case "running":
            return root.primaryAccent
        case "attention":
            return root.attentionStatus
        case "failed":
            return root.failureStatus
        case "stale":
            return root.staleStatus
        case "offline":
        case "unknown":
        default:
            return root.unknownStatus
        }
    }

    readonly property FontLoader rajdhaniLight: FontLoader { source: "fonts/Rajdhani-Light.ttf" }
    readonly property FontLoader rajdhaniRegular: FontLoader { source: "fonts/Rajdhani-Regular.ttf" }
    readonly property FontLoader rajdhaniMedium: FontLoader { source: "fonts/Rajdhani-Medium.ttf" }
    readonly property FontLoader rajdhaniSemiBold: FontLoader { source: "fonts/Rajdhani-SemiBold.ttf" }
    readonly property FontLoader jetBrainsLight: FontLoader { source: "fonts/JetBrainsMono-Light.ttf" }
    readonly property FontLoader jetBrainsRegular: FontLoader { source: "fonts/JetBrainsMono-Regular.ttf" }
    readonly property FontLoader jetBrainsMedium: FontLoader { source: "fonts/JetBrainsMono-Medium.ttf" }

}
