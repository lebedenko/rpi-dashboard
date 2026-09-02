# Frame rendering on rotated outputs

## Context

The Waveshare display exposes a native 320×1480 mode and Cage rotates it by 270 degrees to the
dashboard's 1480×320 landscape geometry. With Qt 6.8.2, zero-radius `PathArc` elements used for
chamfered corners corrupt closed `Frame` paths on that rotated output: nominally vertical edges
drift horizontally and each frame appears trapezoidal. Other layout coordinates and non-frame
content remain correct.

## Observable acceptance criteria

- Frame edges follow their requested path on the rotated Raspberry Pi output; vertical edges do
  not drift horizontally between their endpoints.
- Chamfered and square corners use explicit line segments; arc commands are emitted only for
  rounded corners with a positive radius.
- Frame corner geometry, fill, border containment, colors, thickness, and public API remain
  unchanged.
- Separator and weather graph rendering remain unchanged.

## Non-goals

- Changing layout dimensions, display rotation, scale, colors, or typography.
- Changing the selected Qt Quick Shapes renderer or global scene-graph backend.
- Replacing Qt Quick Shapes with custom C++ rendering.

## Verification

Run the focused frame and dashboard QML tests, `task test`, and `task check`. Install the candidate
on the Raspberry Pi and capture the Cage output to confirm that frame edges are no longer sheared.
