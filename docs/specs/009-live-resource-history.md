# Live Resource History

## Goal

Show truthful recent CPU and physical-memory utilization in the expanded local-device card without blocking the GUI thread or inventing data before launch.

## Observable contract

- `SysMetricsService` owns a stable list model with `elapsedMilliseconds`, optional `cpuUsageRatio`, and optional `memoryUsageRatio` roles.
- Every completed collection attempt appends one sample on the service thread. Ready and partial attempts retain each available ratio; failed attempts append an unavailable sample so both series have a gap.
- Samples more than 60 seconds older than the latest completed attempt are removed. History starts empty and is not persisted.
- Failed attempts do not replace the last successful current snapshot or its timestamp.
- The expanded card plots the rolling window from −60 seconds through now. Sample positions use their monotonic completion timestamps, and missing values break only the affected series.
- CPU is a two-pixel `#36B9FF` blue line and memory is a two-pixel `#A66CFF` purple line on the `#041321` plot background. Before curve construction, each uninterrupted run is adaptively filtered from its clamped raw ratios. CPU uses a time constant interpolated from 1.8 seconds to 0.35 seconds as absolute raw change grows from 3% to 25%; memory uses 3.0 seconds. Filtering uses `alpha = 1 - exp(-dt / time_constant)`, starts each run at its first value, updates immediately for non-positive time deltas, and resets after a missing value.
- Filtered runs are drawn with C1-continuous, shape-preserving cubic interpolation through every filtered sample. Tangents become zero at extrema, and duplicate or non-increasing timestamps use a linear fallback. Cubic controls remain inside the adjacent filtered samples' time interval and value extrema, so smoothing cannot invent a local minimum or maximum.
- Filtering and smoothing are visual only. They do not add telemetry samples, connect across missing values, or change the truthful 60-second sample history. CPU and memory gaps and filter state are independent.
- Each series has a subtle area fill that fades from 10% series opacity at the curve to transparent at the baseline, a five-pixel glow at 4% opacity, a three-pixel anti-aliased fringe at 12% opacity, and a crisp two-pixel core. All fills are drawn before all glow, fringe, and core strokes so a later fill cannot obscure the other metric.
- The plot reserves six pixels of vertical headroom and a fixed 10-pixel right endpoint zone. The complete 60-second curve window maps into the remaining drawable width. Cubics are tessellated at approximately one point per logical pixel, with at most 64 subdivisions per sample interval. Each stroke is a vertex-colored ribbon with shared averaged joins and transparent outer vertices feathering the five-pixel glow, three-pixel fringe, and two-pixel core. A persistent rectangular scene-graph clip at the endpoint-zone boundary clips every fill, glow, fringe, and core layer. Window-wide multisampling is not enabled.
- Before front rows are removed, the renderer retains every removed sample that is still inside the currently displayed 60-second window plus the closest preceding sample needed for clipping and filter continuity. This ordered prefix is merged with the current model snapshot and survives interrupted or irregular transitions. Entries are discarded only after they are safely left of the displayed cutoff. Missing values remain genuine, metric-independent gaps. Startup remains empty until telemetry exists and no history is fabricated or extended.
- At the endpoint-zone boundary, each metric whose active cubic intersects `NOW` receives a horizontal two-pixel core join from local X=0 through X=2 and an outlined radius-four ring centered at local X=6. The assembly is translated only to the boundary and the intersection Y, so its attachment is independent of incoming curve angle. It is hidden only when `NOW` lies in a genuine missing-sample gap and reaches the true newest filtered value without snapping on the final frame.
- The grid contains only faint solid horizontal guides at 100%, 75%, 50%, and 25%. The solid 0% baseline is approximately twice as strong; there are no vertical guides or left axis. Axis and legend text use `#8295AC`, and legend swatches are short two-pixel line samples.
- When the newest timestamp advances, a `QVariantAnimation` moves the 60-second window end from its current interpolated position to the new timestamp over 350 ms with smoothstep easing. An active transition retains the duration with which it started. Interrupted transitions rebuild once at the displayed position and continue toward the new target. Pruning, same-timestamp data changes, model replacement, dimensions, colors, and background rebuild only when needed without restarting scrolling; duration changes apply to future transitions. Curve geometry uses static buffers beneath a persistent transform node, so each ordinary animation frame changes only its horizontal matrix and the two endpoint transforms/visibility. Repainting stops when the transition completes, and there is no continuous idle scrolling.

## Non-goals

- Remote-device history, persistence, protocol changes, fabricated startup history, gap bridging, or treating missing values as zero.
- Metric selection, per-core series, GPU history, chart interaction, or alternate units and axes.
- Continuous repainting between one-second collection attempts.

## Acceptance criteria

- Deterministic model tests cover ordered appends, independently optional ratios, failed-attempt gaps, and the exact 60-second pruning boundary.
- Service tests confirm failed attempts append gaps while preserving the last successful snapshot and timestamp.
- Renderer geometry tests confirm deterministic CPU and memory filtering, C1 tangent continuity, no overshoot at monotonic sections or extrema, nonuniform and duplicate timestamp behavior, dense bounded tessellation, truthful independent metric gaps and filter resets, exact −60-second clipping from a retained prefix, ratio clamping, headroom, true-`NOW` mapping to the endpoint-zone boundary, window-relative curve intersections, and feather alpha. Renderer signal tests deterministically drive animation time and confirm irregular multi-row pruning, advancing and interrupted transitions, non-advancing updates, retained active duration, horizontal endpoint assembly geometry, and root, clip, transform, and geometry-buffer reuse between frames.
- QML tests confirm horizontal-only guides, the stronger baseline, line-sample legends, renderer properties, stable model forwarding, expanded-content lifetime, focus preservation, and 1480×320 bounds.
- A 1480×320 render matches `docs/mockups/m4.png`: smooth thin blue and purple lines, subtle fills and glow, endpoint rings, restrained labels, horizontal guides, and a stronger baseline.
- A matching 1480×320 profiling capture of graph transitions has p50 frame time at most 18.5 ms, p95 at most 25 ms, and no frames over 33 ms. Visual inspection confirms continuous motion, persistent endpoint rings, smooth steep edges, correct gaps, and no left-edge flicker. Final performance and visual acceptance is repeated on Raspberry Pi 5 hardware.

## Verification

- Run focused `system_metrics_test`, `dashboard_qml_test`, and `dashboard_startup_test` targets.
- Run `task test` and `task check`.
- Inspect a 1480×320 render locally and repeat hardware-accelerated validation on Raspberry Pi.
