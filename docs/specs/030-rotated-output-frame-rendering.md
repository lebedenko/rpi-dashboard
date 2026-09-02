# Frame rendering on rotated outputs

## Context

The Waveshare display exposes a native 320×1480 mode and Cage rotates it by 270 degrees to the
dashboard's 1480×320 landscape geometry. With Qt 6.8.2, zero-radius `PathArc` elements used for
chamfered corners corrupt closed `Frame` paths on that rotated output: nominally vertical edges
drift horizontally and each frame appears trapezoidal. Other layout coordinates and non-frame
content remain correct.

Qt 6.8.2's synchronous `Shape.CurveRenderer` can commit stale path geometry when layout supplies
an item's dimensions across multiple synchronization rounds before `updateNode()`. The fixed-width
current and hourly weather panels receive their width before their layout-derived height, while the
daily panel receives both dimensions from layout. Their frames can consequently retain geometry
from a provisional size. Hourly and daily separators can retain stale endpoint geometry through the
same lifecycle. This is the behavior corrected upstream by QTBUG-133267.

## Observable acceptance criteria

- Frame edges follow their requested path on the rotated Raspberry Pi output; vertical edges do
  not drift horizontally between their endpoints.
- Chamfered and square corners use explicit line segments; arc commands are emitted only for
  rounded corners with a positive radius.
- Full-size current-conditions, hourly, and daily weather-panel frames visibly render their top,
  bottom, left, right, and four chamfer edges.
- Full-length hourly vertical and daily horizontal separators render within their panels after
  layout settles.
- Curve-rendered frames and separators use asynchronous Shape processing so changes that arrive
  during geometry processing are reprocessed on Qt 6.8.2.
- Frame corner geometry, fill, border containment, colors, thickness, and public API remain
  unchanged.
- Separator and weather graph rendering remain unchanged.

## Non-goals

- Changing layout dimensions, display rotation, scale, colors, or typography.
- Changing the selected Qt Quick Shapes renderer or global scene-graph backend.
- Replacing Qt Quick Shapes with custom C++ rendering.
- Changing the weather layout, graph implementation, path definitions, or rotation configuration.

## Verification

Run the focused Frame, Separator, and dashboard QML tests, `task test`, `task check`, and
`git diff --check`. Build and run the native tests against Qt 6.8.2 on the Raspberry Pi, install
through the existing installer, and confirm the dashboard service, Cage, process tree, and journal
remain healthy. Capture the active 1480×320 weather page and confirm every weather-panel edge and
chamfer is present; hourly and daily separators span their panels; and content positions, graph
lines, touch layout, sidebars, and the outer page frame are unchanged.
