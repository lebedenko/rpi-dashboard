# Reusable frame component

## Context

Dashboard surfaces need a shared decorative frame that can express square, rounded, chamfered,
and mixed corners without allowing an opaque background to extend outside the frame geometry.

## Observable API

- `color` controls the border color and defaults to `Theme.passiveBorder`.
- `lineWidth` controls the border thickness and defaults to 1 px.
- `backgroundColor` controls the frame fill and defaults to transparent.
- `corners` defaults to square corners. It accepts a uniform `{ rounded: size }` or
  `{ chamfered: size }` configuration, or an object with `topLeft`, `topRight`, `bottomRight`, and
  `bottomLeft` entries that each contain either `rounded` or `chamfered`.
- Missing per-corner entries are square. Negative, non-finite, malformed, and oversized corner
  sizes produce valid, non-intersecting geometry. Nested changes to `corners` take effect after the
  caller reassigns the configuration object.

## Observable acceptance criteria

- The component draws one closed path whose edges proceed clockwise.
- Square, circular rounded, straight chamfered, and independently mixed corners render according
  to the supplied configuration.
- Every corner size is limited to half the smaller current frame dimension, and resizing recomputes
  that limit.
- The border and optional background use the same path. An opaque background fills the center but
  leaves regions outside rounded or chamfered corners transparent.
- Custom border color and thickness are applied without changing the frame geometry API.
- The component is registered in the `Rpi.Dashboard` QML module and passes QML linting and focused
  Qt Quick Tests.
- Dashboard frame paths and framed panel or control backgrounds use this component while preserving
  their existing geometry, colors, focus and selection states, sizing, accessibility, and interaction.
- The device status badge uses square left corners and two 12 px right-side chamfers, forming a
  six-vertex frame.

## Non-goals

- Clipping or masking child content.
- Adding C++ geometry, networking, parsing, collection, or state.
- Supporting corner shapes other than square, circular rounded, and straight chamfered corners.

## Verification

Run `qmllint` on the component and its focused test, run `dashboard_qml_test`, then run `task test`
and `task check`.
