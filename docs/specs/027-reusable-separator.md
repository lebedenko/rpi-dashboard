# Reusable pixel-aligned separator

## Scope

Provide a reusable decorative separator for the `Rpi.Dashboard` QML module and use it for every
structural divider in the weather experience, including the contextual rail.

## Acceptance criteria

- `Separator` supports horizontal and vertical orientation, solid and dotted line styles, a color,
  opacity, and an integer width expressed in physical pixels.
- The separator uses the Qt Quick Shapes curve renderer. Its stroke center is aligned in
  scene/window coordinates and its width maps to the physical-pixel grid at integer and fractional
  display scale factors.
- Dotted lines use the existing `[1, 3]` dash pattern and round caps.
- The current-conditions horizontal divider is solid, spans the existing content width, and keeps
  its existing separator area.
- Current-metric and hourly-forecast column dividers are vertical and dotted, preserving their
  existing spans.
- The divider below the date and the two weather-rail section dividers are horizontal and dotted,
  preserving their existing widths.
- All weather-experience separators use `Theme.sectionDividerStrong`, one physical pixel, and
  opacity `0.5`.
- The five daily forecast rows are derived from snapped cumulative boundaries, differ in height by
  no more than one physical pixel, and consume all space below the title.
- Daily row separators are horizontal and solid, retain equal 8 px side margins, and have a
  one-physical-pixel stroke at 1.0 and 1.25 scale factors.
- Shapes-based separator rendering remains isolated in reusable rendering components.

## Non-goals

- Migrating separators outside `WeatherPage.qml` and `ClockSidebar.qml`.
- Changing daily forecast content, colors, spacing, or panel dimensions.
