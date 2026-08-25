# Vibrant Mockup-Inspired Theme

## Goal

Bring the dashboard closer to the supplied HoloNight-inspired mockups with deep navy surfaces and luminous cyan, blue, purple, and green accents while preserving the existing information hierarchy and interaction model.

## Observable contract

- The dashboard background is `#020A13`; general, elevated, raised, card, and selected surfaces are respectively `#061321`, `#0A1A2B`, `#10283C`, `#041321`, and `#09283C`.
- Primary, secondary, and muted text are respectively `#F2F7FC`, `#B7C7D9`, and `#8295AC`. Chart text uses the muted text color.
- Primary interaction, focus, card framing, CPU history, memory history, and online status use respectively `#19D3F3`, `#5DE7FF`, `#20D4F7`, `#36B9FF`, `#A66CFF`, and `#50F0A0`.
- Borders, dividers, rails, badges, detail rails, and action surfaces remain navy-blue variants attached to their existing semantic theme roles. Faint chart guides preserve their existing 10% and 20% alpha hierarchy.
- Source SVGs use matching fallback colors for navigation, detail, selected-action, and disabled-action content. QML tinting remains authoritative when an icon is displayed through a control.
- Normal text has a contrast ratio of at least 4.5:1 against its adjacent surface. Focus and active interactive indicators have a contrast ratio of at least 3:1 against adjacent surfaces.
- At 1480×320, the device card and resource graph visually follow `docs/mockups/m1.png` and `docs/mockups/m4.png`: blue-black foundations, crisp cyan framing, blue CPU, purple memory, and green online status.

## Non-goals

- Layout, spacing, typography, navigation, interaction, animation, telemetry, and semantic token changes.
- New public theme tokens, gradients, shadows, blur, full-screen glow, or a dependency on HoloNight or `holonight-qt`.
- Recoloring warning or error meanings, introducing additional themes, or changing SVG geometry.

## Acceptance criteria

- QML tests lock the core palette and confirm the sidebar, device card, graph, status, and action controls continue to consume semantic theme tokens.
- Resource-history tests lock the renderer defaults to the QML CPU, memory, and plot-background colors.
- A 1480×320 render is compared with all four mockups, with the device card and graph assessed against `m1.png` and `m4.png` in particular.
- Selected, focused, pressed, disabled, online, chart, and empty-page states remain legible.

## Verification

- Run focused `dashboard_qml_test`, `resource_history_series_test`, and `dashboard_startup_test` targets.
- Run `task test` and `task check`.
- Inspect a local 1480×320 render and repeat final visual validation on the Raspberry Pi display when available.
