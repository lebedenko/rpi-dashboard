# Live Resource History

## Goal

Show truthful recent CPU and physical-memory utilization in the expanded local-device card without blocking the GUI thread or inventing data before launch.

## Observable contract

- `SysMetricsService` owns a stable list model with `elapsedMilliseconds`, optional `cpuUsageRatio`, and optional `memoryUsageRatio` roles.
- Every completed collection attempt appends one sample on the service thread. Ready and partial attempts retain each available ratio; failed attempts append an unavailable sample so both series have a gap.
- Samples more than 60 seconds older than the latest completed attempt are removed. History starts empty and is not persisted.
- Failed attempts do not replace the last successful current snapshot or its timestamp.
- The expanded card plots the rolling window from −60 seconds through now. Sample positions use their monotonic completion timestamps, and missing values break only the affected series.
- CPU is a two-pixel `#2F9BFF` blue line and memory is a two-pixel purple line. Both use consistent ribbon geometry and have a barely visible three-pixel downward glow that fades from low opacity at the line edge to fully transparent. Plot geometry is clipped to the plot bounds.
- Major gridlines are quiet short dashes. The left and bottom axis edges are thin, solid, and slightly stronger than the grid.
- The chart is non-interactive and has no continuous animation.

## Non-goals

- Remote-device history, persistence, protocol changes, interpolation, fabricated startup history, or treating missing values as zero.
- Metric selection, per-core series, GPU history, chart interaction, or alternate units and axes.
- Continuous repainting between one-second collection attempts.

## Acceptance criteria

- Deterministic model tests cover ordered appends, independently optional ratios, failed-attempt gaps, and the exact 60-second pruning boundary.
- Service tests confirm failed attempts append gaps while preserving the last successful snapshot and timestamp.
- QML tests confirm the history model is forwarded without replacement, multiple CPU and memory samples including missing-value gaps preserve the renderer, the renderer follows expanded-content lifetime, labels and plot bounds remain correct, and model updates preserve the card, expansion, and keyboard focus.
- A 1480×320 render shows thin blue and purple lines, a barely visible downward glow, subtle dashed gridlines, clear labels, and solid left/bottom axes.

## Verification

- Run focused `system_metrics_test`, `dashboard_qml_test`, and `dashboard_startup_test` targets.
- Run `task test` and `task check`.
- Inspect a 1480×320 render locally and repeat hardware-accelerated validation on Raspberry Pi.
