# Right Status Sidebar with Clock

## Context

Add a dedicated status rail to the right side of the 1480×320 dashboard while preserving the existing left-side navigation and page behavior. The first status content is the Raspberry Pi's local time and date.

## Functional requirements

- The status sidebar is 144 logical pixels wide and inset 10 px from the physical top, right, and bottom edges.
- Its frame mirrors the existing navigation sidebar surface, border, chamfers, and rounded top-right corner.
- A centered 28 px time appears near the top of the rail, with a centered 14 px date directly below it.
- Time and date use the system locale's short formats and the system timezone.
- The displayed timestamp refreshes every second so minute, date, timezone, and system-clock changes appear promptly.
- The combined time and date are exposed as non-interactive accessible static text. The individual visual labels are not separate accessibility stops.
- The status sidebar does not accept keyboard focus or change existing Left, Right, Home, F5, pointer, or touch navigation behavior.
- Sidebar width and clock typography are semantic theme tokens. Existing surface, border, spacing, font, and text-color roles are reused.

## Acceptance criteria

- At 1480×320, the status sidebar is 144×300 at (1326, 10), and the page stack is 1252 px wide at x=74.
- Both clock labels remain within the 10 px display-safe inset, with the time above the date.
- The time string equals `Qt.formatTime(timestamp, Locale.ShortFormat)` and the date string equals `Qt.formatDate(timestamp, Locale.ShortFormat)` for the component's current timestamp.
- The time uses `Theme.clockTimeTextSize` and `Theme.textPrimary`; the date uses `Theme.clockDateTextSize` and `Theme.textSecondary`.
- Accessibility exposes one `Accessible.StaticText` description containing the displayed time and date.
- The left navigation retains four 48×48 touch targets and all existing page selection and focus behavior.
- `ClockSidebar.qml` is registered in the `HoloNight.Dashboard` QML module without a new dependency or C++ API.

## Non-goals

- Weather, connectivity, health indicators, settings, or interactions in the status sidebar.
- A seconds-specific display; the locale's short time representation is authoritative.
- Changes to page content, navigation order, keyboard shortcuts, telemetry, or protocol types.
- A HoloNight runtime dependency or use of private Qt APIs.

## Verification

Run `dashboard_qml_test` and `dashboard_startup_test` first, followed by `task test` and `task check`. Inspect all pages with `task run-windowed` at 1480×320 for clipping, localized text fit, frame continuity, and card readability.
