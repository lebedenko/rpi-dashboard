# Theme token cohesion

## Context

The dashboard theme accumulated component-named chamfer and radius values plus repeated status-color
selection. Consolidate those values into a small, typed design scale while keeping the singleton flat
and preserving the established 1480×320 presentation.

## Observable acceptance criteria

- `Theme` exposes small, medium, and large chamfers of 6, 8, and 12 px and radii of 2, 4, and 8 px.
- Rectangular surfaces use the shared geometry scale; circles and pills derive their radius from their
  own dimensions.
- Project rows normalize from a 5 px to a 6 px chamfer, and Weather panels normalize from a 3 px to a
  4 px radius. Other geometry and interaction remain unchanged.
- `Theme.statusColor(status)` maps online and healthy to online green; registered and running to the
  primary accent; attention, failed, and stale to their semantic roles; and offline, unknown, and
  unexpected values to muted text.
- Equal semantic roles bind to one canonical color source, and chart grid and axis colors retain their
  existing muted-text alpha values.
- All bundled font loaders are read-only, font-family discovery is typed, and bundled-font readiness
  and asynchronous fallback behavior remain unchanged.
- Removed component-named geometry tokens have no remaining in-repository consumers.
- The dashboard still initializes without QML errors at 1480×320.

## Non-goals

- A configurable universal shape component or nested theme groups.
- Expanding typography roles or relocating component-private layout metrics.
- Changing font-loading lifecycle behavior.
- Addressing unrelated QML lint findings.

## Verification

Run the focused dashboard QML and startup tests, lint modified QML, then run `task test` and
`task check`. Compare a 1480×320 render with the previous design, paying particular attention to
project rows and Weather panels. Record physical Raspberry Pi display validation separately.
