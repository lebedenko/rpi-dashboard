# Live Resource History

## Goal

Show truthful recent CPU and physical-memory utilization in the expanded local-device card without blocking the GUI thread or inventing data before launch.

## Observable contract

- `SysMetricsService` owns a stable list model with `elapsedMilliseconds`, optional `cpuUsageRatio`, and optional `memoryUsageRatio` roles.
- Every completed collection attempt appends one sample on the service thread. Ready and partial attempts retain each available ratio; failed attempts append an unavailable sample so both series have a gap.
- Samples more than 60 seconds older than the latest completed attempt are removed. History starts empty and is not persisted.
- Failed attempts do not replace the last successful current snapshot or its timestamp.
- The expanded card plots the rolling window from −60 seconds through now. Sample positions use their monotonic completion timestamps, and missing values break only the affected series.
- CPU is a two-pixel `#35A7FF` blue line and memory is a two-pixel `#A875F5` purple line. Each uninterrupted run is drawn with C1-continuous, shape-preserving cubic interpolation through every real sample. Tangents become zero at extrema, and duplicate or non-increasing timestamps use a linear fallback. Cubic controls remain inside the adjacent samples' time interval and value extrema, so smoothing cannot invent a local minimum or maximum.
- Smoothing is visual only. It does not add telemetry samples, connect across missing values, or change the truthful 60-second sample history. CPU and memory gaps are independent.
- Each series has a subtle area fill that fades from 10% series opacity at the curve to transparent at the baseline, a restrained six-pixel glow at 7% opacity, and a crisp two-pixel stroke. All fills are drawn before all glows and strokes so a later fill cannot obscure the other metric.
- The plot reserves six pixels of vertical headroom, clamps ratios to `[0, 1]`, and clips all generated geometry to its bounds. Cubics are tessellated by horizontal distance at roughly one point per two pixels, with at most 32 subdivisions per sample interval. Each stroke is one continuous ribbon with shared averaged joins, a one-pixel alpha fringe, and anti-aliased edges.
- When pruning removes the closest real sample before −60 seconds, the renderer retains that predecessor in both crossfade snapshots. If it and the oldest retained sample both provide a metric, their real curve is clipped exactly at −60 seconds so the stroke starts at the left plot edge without flicker. An unavailable value on either side preserves a genuine gap. Startup remains empty until telemetry exists and no history is fabricated or extended.
- The newest sample receives an outlined endpoint ring for each metric present in that sample. A stale run ending before the newest sample never receives a current marker.
- The grid contains only faint solid horizontal guides at 100%, 75%, 50%, and 25%. The solid 0% baseline is approximately twice as strong; there are no vertical guides or left axis. Axis and legend text use `#71859B`, and legend swatches are short two-pixel line samples.
- A complete previous graph snapshot crossfades to a new sample snapshot over 200 ms using scene-graph opacity nodes. Model replacement, dimensions, colors, background, and transition-duration changes repaint immediately without animation. The chart remains non-interactive and has no continuous animation between sample changes.

## Non-goals

- Remote-device history, persistence, protocol changes, fabricated startup history, gap bridging, or treating missing values as zero.
- Metric selection, per-core series, GPU history, chart interaction, or alternate units and axes.
- Continuous repainting between one-second collection attempts.

## Acceptance criteria

- Deterministic model tests cover ordered appends, independently optional ratios, failed-attempt gaps, and the exact 60-second pruning boundary.
- Service tests confirm failed attempts append gaps while preserving the last successful snapshot and timestamp.
- Renderer geometry tests confirm exact sample passage, C1 tangent continuity, no overshoot at monotonic sections or extrema, nonuniform and duplicate timestamp behavior, dense bounded tessellation, truthful metric gaps, exact −60-second clipping from a real predecessor, ratio clamping, headroom, and endpoint-ring eligibility. Renderer signal tests confirm front-row removal captures the closest predecessor and preserves it in both crossfade snapshots.
- QML tests confirm horizontal-only guides, the stronger baseline, line-sample legends, renderer properties, stable model forwarding, expanded-content lifetime, focus preservation, and 1480×320 bounds.
- A 1480×320 render matches `docs/mockups/m4.png`: smooth thin blue and purple lines, subtle fills and glow, endpoint rings, restrained labels, horizontal guides, and a stronger baseline.

## Verification

- Run focused `system_metrics_test`, `dashboard_qml_test`, and `dashboard_startup_test` targets.
- Run `task test` and `task check`.
- Inspect a 1480×320 render locally and repeat hardware-accelerated validation on Raspberry Pi.
