pragma Singleton
import QtQuick

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

    readonly property int spacingSmall: 8
    readonly property int spacingMedium: 16
    readonly property int spacingLarge: 24
    readonly property int touchTarget: 56
    readonly property int displaySafeInset: 11

    function preferredFixedFontFamily(): string {
        if (installedFontFamilies.includes("JetBrains Mono"))
            return "JetBrains Mono"
        if (installedFontFamilies.includes("IBM Plex Mono"))
            return "IBM Plex Mono"
        return "monospace"
    }
}
