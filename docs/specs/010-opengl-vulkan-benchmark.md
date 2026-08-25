# OpenGL and Vulkan Rendering Benchmark

## Context and goal

The Raspberry Pi 5 dashboard needs an evidence-based Qt Quick rendering-backend choice. The measurement must represent the production 1480x320 Cage/Wayland kiosk rather than an SSH, desktop, headless, or software-rendered session.

## Observable acceptance criteria

- A separate Ninja `RelWithDebInfo` build is configured in `build/profile` with `QT_QML_DEBUG`; tracked CMake and launcher behavior remain unchanged.
- Four clean 90-second profiles run in OpenGL, Vulkan, Vulkan, OpenGL (ABBA) order with the same display transform and keyboard workload.
- OpenGL diagnostics identify hardware V3D rendering. Vulkan diagnostics identify the Broadcom V3DV physical device. Any `llvmpipe`, software Qt backend, failed Vulkan initialization, abnormal exit, wrong geometry, or throttled run is rejected.
- Before runs 2-4, CPU temperature returns to within 2 C of run 1's starting temperature.
- Each accepted run records a `.qtd` trace, Qt scene-graph diagnostics, `perf stat` counters, one-second per-process CPU/RSS samples (from which maximum dashboard RSS is derived), and one-second CPU temperature/frequency samples under a timestamped `profiler/runs/` directory.
- Each trace gets a standalone report covering frame p50/p95/p99/max, frames over 25/33/50 ms, scene-graph and painting cost, pixmap-cache behavior, and project hotspots.
- An aggregate report shows both raw runs and backend medians, normalizes CPU counters by elapsed wall time, describes variability, and recommends OpenGL or Vulkan using the decision rule below.

## Fixed workload

- 0-30 s: expanded Overview, untouched.
- 30/35/40 s: Right to Systems, Projects, and Weather.
- 45 s: Home.
- 50 s: F5; 52 s: Space to collapse the card; 57 s: Space to expand it.
- 62/67/72 s: Right through the placeholder pages.
- 77 s: Home; leave Overview active until 90 s.
- Press Ctrl+Q at 90 s for a normal exit so the trace is flushed.

## Decision rule

Vulkan is a material improvement only when both Vulkan runs agree directionally and either p95/p99 improve by at least 10% with no more than 5% higher normalized CPU cost, or normalized CPU cost improves by at least 10% without worsening p95/p99 or jank counts by more than 5%. Otherwise classify the result as equivalent, mixed, or an OpenGL advantage. Temperature is supporting evidence only.

## Non-goals

- Changing application source, public APIs, QML, CMake configuration, or kiosk launcher behavior.
- Comparing software renderers or benchmarking outside Cage/Wayland.
- Changing permissions or device groups to obtain `vcgencmd` throttling data.
- Treating temperature alone as the backend-selection metric.

## Verification

Run `profiler/run-abba.sh` from an active physical TTY. Retain the four run directories and generated reports, verify all validation markers are `accepted`, then produce the aggregate comparison. A study without four accepted, cleanly exited traces is incomplete.
