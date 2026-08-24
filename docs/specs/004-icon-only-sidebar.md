# Icon-Only Chamfered Sidebar

## Context

The dashboard sidebar should match the compact visual language of `docs/mockups/m2.png` while preserving the existing four-page navigation and full-bleed treatment required by the rounded display.

## Functional requirements

- The sidebar is an 88 logical-pixel-wide full-height surface.
- The sidebar contains no visible title or navigation labels. Each navigation button retains a translated accessible name.
- Four 56×56 icon-only buttons are vertically centered with 16 px spacing. Their visible frames remain inside the 11 px display-safe inset.
- Overview is selected at startup. Touch/click selection, tab focus, Left, Right, Home, and F5 behavior remain unchanged.
- Each button uses a 24×24 tintable outline SVG: a panel grid for Overview, pulse waveform for Systems, terminal prompt for Projects, and cloud/sun for Weather.
- Default, hover, pressed, selected, and keyboard-focus states are visually distinct. Selection remains visible after keyboard focus leaves.
- Button frames use opposing chamfered corners. Selected buttons use a 1 px cyan/blue frame; focused buttons use a 2 px focus-accent frame.
- The sidebar boundary draws only a 12 px top-right chamfer followed by the internal right border. It draws no top, bottom, or physical-left border.
- Optional delayed tooltips may reveal translated navigation names to pointer and keyboard users.

## Acceptance criteria

- The sidebar width is exactly 88 px, with four centered 56×56 targets separated by 16 px.
- No visible `Dashboard`, `Overview`, `Systems`, `Projects`, or `Weather` label is rendered in the sidebar.
- All four icon resources initialize through the dashboard QML module and are dynamically tinted through the button icon API.
- Each button exposes its translated page name to accessibility and activates the corresponding page.
- The separator consists only of the top-right chamfer and internal vertical edge.
- The focused startup test, `task test`, and `task check` pass.
- At 1480×320 and on the physical display, button frames are unobscured and all interaction states and navigation paths are distinct and functional.

## Non-goals

- Changing page headings, page content, navigation order, or keyboard shortcuts.
- Adding bitmap or duplicate per-state icon assets.
- Adding C++ APIs, dependencies, private Qt modules, or a HoloNight runtime dependency.

## Verification

Run `dashboard_startup_test` first, followed by `task test` and `task check`. Inspect the dashboard at 1480×320 and on the validated rounded display, including selected, hovered, pressed, and focused states for every button.
