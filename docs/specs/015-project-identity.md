# Project identity

## Intent

Use `rpi-dashboard` consistently as the project identity in user-visible text, build targets,
runtime artifacts, installation paths, service names, diagnostics, and internal module namespaces.

## Observable acceptance criteria

- The dashboard executable is named `rpi-dashboard` and the telemetry agent is named
  `rpi-dashboard-agent`.
- The installed systemd unit is `rpi-dashboard.service`, and all installed paths, provisioning
  commands, launcher defaults, and operational documentation use that name.
- CMake targets and aliases use the `rpi-dashboard` or `RpiDashboard` identity as appropriate.
- The QML module URI is `Rpi.Dashboard`, and startup and tests load that URI.
- User-visible application names, diagnostics, temporary paths, profiler traces, and the HTTP user
  agent use the `rpi-dashboard` identity.
- A case-insensitive repository scan finds no legacy-brand occurrence except statements that
  explicitly prohibit a legacy runtime, build, library, module, plugin, icon, configuration,
  resource, or package dependency.

## Non-goals

- Changing the dashboard account name.
- Changing behavior, architecture, visual styling, or telemetry protocol semantics.
- Removing documentation that explicitly prohibits the legacy dependency.
