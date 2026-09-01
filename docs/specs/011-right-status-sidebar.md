# Right Status Sidebar with Clock

## Context

Add a dedicated status rail to the right side of the 1480×320 dashboard while preserving the existing left-side navigation and page behavior. The first status content is the Raspberry Pi's local time and date.

## Functional requirements

- The status sidebar is 144 logical pixels wide and inset 10 px from the physical top, right, and bottom edges.
- Its frame mirrors the existing navigation sidebar surface and border, with a rounded top-left corner and chamfered top-right, bottom-right, and bottom-left corners.
- A centered 28 px time appears near the top of the rail, with a centered 14 px date directly below it.
- Time uses the system locale with the `hh:mm` format, date uses the system locale with the `ddd dd MMM` format, and both use the system timezone.
- The displayed timestamp refreshes every second so minute, date, timezone, and system-clock changes appear promptly.
- The combined time and date are exposed as non-interactive accessible static text. The individual visual labels are not separate accessibility stops.
- An explicit page context controls contextual content: Overview and System are empty, Projects
  shows CI and runner state, and Weather shows exactly three dotted-separated sections: air,
  next-solar-event, and precipitation. Time and date remain visible everywhere.
- Weather context content is left-aligned within equal side margins. Its labels and values use the
  sans font and body text size; section labels and the AQI number use the primary accent. AQI
  category colors map native indices 1–2 to online, 3 to attention, and 4–5 to failure. Solar time
  uses primary text. A positive remaining-day precipitation probability retains its classified
  `RAIN`, `SNOW`, `MIXED`, or `PRECIPITATION` label and numeric percentage. Zero probability is
  presented as `PRECIPITATION` with `NONE`. The label/value lines within each Weather section use
  a compact 2 px vertical gap.
- The status sidebar itself does not accept keyboard focus or change existing Left, Right, Home,
  F5, pointer, or touch navigation behavior.
- Sidebar width and clock typography are semantic theme tokens. Both clock labels use the existing sans-serif font role, and existing surface, border, spacing, and text-color roles are reused.

## Acceptance criteria

- At 1480×320, the status sidebar is 144×300 at (1326, 10), and the page stack is 1252 px wide at x=74.
- Both clock labels remain within the 10 px display-safe inset, with the time above the date.
- The time string equals `Qt.formatTime(timestamp, "hh:mm")` and the date string equals `Qt.formatDate(timestamp, "ddd dd MMM")` for the component's current timestamp.
- The time uses `Theme.clockTimeTextSize` and `Theme.textPrimary`; the date uses `Theme.clockDateTextSize` and `Theme.textSecondary`.
- Both time and date use `Theme.sansFontFamily`.
- Accessibility exposes one `Accessible.StaticText` description containing the displayed time and date.
- The left navigation retains four 48×48 touch targets and all existing page selection and focus behavior.
- `ClockSidebar.qml` is registered in the `Rpi.Dashboard` QML module without a new dependency or C++ API.

## Non-goals

- Connectivity, settings, or additional interactions in the status sidebar.
- Visible weather-provider attribution or links.
- A seconds-specific display.
- Changes to page content, navigation order, keyboard shortcuts, telemetry, or protocol types.
- A HoloNight runtime dependency or use of private Qt APIs.

## Verification

Run `dashboard_qml_test` and `dashboard_startup_test` first, followed by `task test` and `task check`. Inspect all pages with `task run-windowed` at 1480×320 for clipping, localized text fit, frame continuity, and card readability.
