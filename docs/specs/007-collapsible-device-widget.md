# Collapsible Local Device Widget

## Context

Overview needs to present the local Raspberry Pi as a compact device dashboard at the fixed 1480×320 logical display size. The visual treatment follows the stepped, cyan-accented device panel in `docs/mockups/m1.png`, while preserving unknown telemetry as unknown.

## Functional requirements

- Overview renders exactly one local device, numbered `01`, with the hostname uppercased and an `ONLINE` status inferred from the running process.
- The device card starts expanded. Its collapsed header is 64 logical pixels high and its expanded height fills the Overview viewport.
- Activating the 48×48 accessible chevron by pointer, Space, or Enter toggles expansion immediately. F5 focuses that chevron on Overview and the existing placeholder on other pages.
- Header CPU, memory, temperature, and uptime metrics display `—`; no live or fabricated values or progress are shown.
- Expanded details prefer available system information and display `—` for missing OS, kernel, architecture, hardware, CPU, core-count, and RAM values.
- Resource History shows translated CPU and memory legends, static axes/grid, 0–100 vertical labels, and −60s through current horizontal labels. It has no samples, series, timers, or fabricated history.
- The footer contains `SELECT ACTIVE`, `VIEW STREAMS`, `TERMINAL`, and `MORE`. The first remains selected when activated; the others are disabled.
- The Overview list owns `expandedIndex` and permits zero or one expanded card. For two or more entries, an expanded card leaves 24 logical pixels of the following 64-pixel collapsed card visible after an 8-pixel gap.
- Meaningful content remains inside the shared display-safe inset.

## Visual requirements

- The outer card stroke is inset so its full 1-pixel width remains visible, with stepped cyan accent segments at the top and bottom and a quieter inner body frame.
- The 64-pixel header groups device identity, a framed status badge with a status dot, four equal metric cells with subtle dividers, and the 48×48 chevron.
- Expanded content uses approximately 32 percent of its width for Device Details and 68 percent for Resource History, separated by a visible section divider.
- Device Details has a narrow cyan icon rail. Its seven rows are distributed evenly, separated subtly, and use primary values no smaller than 14 pixels.
- Resource History reserves explicit plot insets for its title, dot legends, axes, and labels. The `−60s` and `NOW` endpoint labels remain inside the plot bounds.
- The footer spans the full inner width as four equal chamfered controls. `SELECT ACTIVE` uses a cyan selected surface and check icon; the other three actions use muted outlined disabled surfaces.
- Flat fills, solid strokes, tintable icons, and static geometry are used; the card has no blur, glow, gradient, or geometry animation.

## Public interfaces

- `SysInfoService` exposes read-only bindable scalar projections of hostname, OS, kernel, architecture, hardware, CPU, core counts, and total memory. Missing fields are invalid variants.
- `DeviceCard` receives identity, status, metrics, details, selection, expanded state, and expanded height explicitly; it has no dependency on `SysInfoService`.
- `OverviewPage` accepts a stable role-based device model and explicitly forwards identity, status, selection, metric, and detail roles. It exposes independent `expandedIndex` and `focusedIndex` values plus its current chevron focus target.

## Non-goals

- Remote discovery or remote-device production rendering.
- Connectivity probing, stale-state semantics, telemetry sampling, graph series, persistence, or working footer actions.
- Geometry animation when expanding or collapsing.

## Acceptance criteria

- Complete and partial service snapshots preserve values and absence through scalar projections.
- At 1480×320, the initial local card fills the safe Overview viewport and shows the required identity, status, details, placeholder metrics, history scaffold, and footer states.
- Pointer and keyboard activation collapse and expand the card, update the chevron accessible name, and remove expanded content through conditional loading.
- F5 focuses the current card chevron even after all cards have been collapsed.
- Selecting an already selected local device is idempotent and does not replace the binding to its model role.
- First and middle expanded cards are positioned at the viewport start and leave a 24-pixel next-card peek; the final expanded card fills the viewport.
- Service refreshes update forwarded roles without replacing the card, losing expansion, or losing keyboard focus.
- Footer cells have equal widths across the full inner span, detail/history widths follow the 32/68 split, detail rows are evenly distributed, and chart labels remain within card bounds.
- F5 focus and safe bounds remain correct on all four pages.
- A 1480×320 offscreen or windowed render is inspected against `docs/mockups/m1.png` for hierarchy, framing, spacing, and typography. Physical-panel appearance is verified separately on Raspberry Pi hardware.

## Verification

- Run focused `system_info_test`, `dashboard_qml_test`, and `dashboard_startup_test` targets.
- Run `task test` and `task check`.
- Inspect the offscreen 1480×320 application; physical-panel verification remains required on Raspberry Pi hardware.
