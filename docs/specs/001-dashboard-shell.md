# Dashboard Shell v1

## Context

The Raspberry Pi dashboard needs a complete kiosk runtime chain from a local TTY, without relying on an existing graphical session. Cage owns the KMS/DRM session and launches the dashboard as its single fullscreen Wayland client.

## Functional requirements

- The dashboard requests fullscreen presentation with a 1480×320 design geometry.
- A fixed 184-pixel sidebar selects Overview, Systems, Projects, and Weather.
- Navigation controls show a label and initial mark, expose selected and keyboard-focus states, provide accessible names, and are at least 48 logical pixels high.
- Pages cannot be changed by swiping.
- Every page shows a translated heading and a centered `Not implemented yet` empty state.
- Left and Right select the adjacent page without wrapping, and Home selects Overview.
- F5 focuses the current empty state without changing pages or claiming that data was refreshed.
- Failure to create the root QML object terminates the dashboard with a nonzero status.
- The TTY launcher starts Cage without decorations, explicitly selects Qt Wayland, and leaves startup diagnostics visible on stderr.

## Acceptance criteria

- A native release build succeeds on Raspberry Pi 5 with Qt 6.8 or newer.
- The dashboard starts through Cage from a local TTY and fills the 1480×320 display.
- Touching each sidebar entry selects its corresponding page.
- Left, Right, Home, and F5 behave as specified, including visible keyboard focus.
- Overview, Systems, Projects, and Weather each display `Not implemented yet`.
- Missing Cage, a missing dashboard executable, or root-QML initialization failure returns a nonzero status with a diagnostic visible on the TTY.

## Non-goals

- Live or fabricated telemetry, connectivity state, and refresh behavior.
- Swipe navigation.
- systemd units, autologin, installation packaging, or cross-compilation.
- A dependency on HoloNight libraries, QML modules, plugins, icons, configuration, or runtime resources.

## Verification

- Launch the dashboard with the offscreen Qt platform, verify it remains running after QML initialization, then terminate it cleanly.
- Validate the launcher with `sh -n`.
- Run the focused startup test, followed by `task test` and `task check`.
- On a locally logged-in Raspberry Pi TTY, follow the release and Cage procedure in the README and manually verify fullscreen geometry, touch navigation, keyboard navigation, focus indication, placeholders, VT switching, and console recovery.
