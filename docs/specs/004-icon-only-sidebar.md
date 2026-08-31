# Inset Fully Bordered Icon-Only Sidebar Experiment

## Context

Experiment with an inset, fully bordered sidebar while preserving the compact visual language of `docs/mockups/m2.png` and the existing four-page navigation.

## Functional requirements

- The visible sidebar panel is 64 logical pixels wide and is inset from the physical left, top, and bottom edges by `Theme.displaySafeInset`.
- The sidebar contains no visible title or navigation labels. Each navigation button retains a translated accessible name.
- Four 48×48 icon-only buttons start 8 px below the panel's top edge, with 8 px spacing. Their visible frames remain inside the display-safe area.
- Overview is selected at startup. Touch/click selection, tab focus, Left, Right, Home, and F5 behavior remain unchanged.
- Each button uses a 24×24 tintable outline SVG: a panel grid for Overview, pulse waveform for Systems, terminal prompt for Projects, and a recognizable cloud-with-sun silhouette for Weather. SVG strokes use a visible neutral fallback color if platform tinting is unavailable.
- Default, hover, pressed, selected, and keyboard-focus states are visually distinct. Selection remains visible after keyboard focus leaves.
- Button frames use 8 px chamfers at top-left and bottom-right. Their top-right and bottom-left corners use a 2 px radius. Selected buttons use a very subtle tinted surface and a 1 px cyan/blue frame; focused buttons use a 2 px focus-accent frame.
- One closed path fills the sidebar surface and draws its complete 1 px passive border.
- The top-left, bottom-left, and bottom-right corners use `Theme.chamferLarge`. The top-right corner uses `Theme.radiusMedium` and a circular arc.
- Delayed tooltips reveal translated navigation names only while a hover-capable pointer is over a button; touch activation and keyboard focus do not show or persist a tooltip.

## Acceptance criteria

- At 1480×320 with `Theme.displaySafeInset` equal to 10 px, the panel geometry is 64×300 at (10, 10), and page content begins at x=74.
- The sidebar width is exactly 64 px, with four top-aligned 48×48 targets separated by 8 px.
- Button frames have 8 px horizontal panel padding. The first frame has 8 px top panel padding; the remaining space stays below the final button.
- No visible `Dashboard`, `Overview`, `Systems`, `Projects`, or `Weather` label is rendered in the sidebar.
- All four icon resources initialize through the dashboard QML module and are dynamically tinted through the button icon API.
- SVG decoding is an explicit build and deployment dependency, and startup emits no icon decoding errors.
- The fixed navigation buttons are instantiated directly without dynamic loaders or intermediary binding objects.
- Each button exposes its translated page name to accessibility and activates the corresponding page.
- The sidebar has 12 px chamfers at top-left, bottom-left, and bottom-right, plus a 4 px rounded top-right corner.
- The border is continuous around the closed sidebar path, and the previous separator-only geometry is absent.
- Sidebar margins remain bound to `Theme.displaySafeInset`; changing that token moves and resizes the panel without changing its visible width.
- The focused startup test, `task test`, and `task check` pass.
- At 1480×320 and on the physical display, button frames are unobscured and all interaction states and navigation paths are distinct and functional.

## Non-goals

- Changing page headings, page content, navigation order, or keyboard shortcuts.
- Repeating the physical display calibration or changing its source photographs.
- Adding bitmap or duplicate per-state icon assets.
- Adding C++ APIs, private Qt modules, or a HoloNight runtime dependency.

## Verification

Run `dashboard_startup_test` and `dashboard_qml_test` first, followed by `task test` and `task check`. Inspect every page and selected, hovered, pressed, and focused state with `task run-windowed` at exactly 1480×320. Check border continuity, all four corner shapes, page alignment, and button clearance. Final acceptance requires validation on the rounded physical panel because a laptop cannot reproduce glass clipping.
