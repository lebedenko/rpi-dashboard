# Dashboard Shell v1

## Context

The Raspberry Pi dashboard needs a complete kiosk runtime chain from a local TTY, without relying on an existing graphical session. Cage owns the KMS/DRM session and launches the dashboard as its single fullscreen Wayland client. The target panel advertises a native 320×1480 portrait mode and must be transformed into the dashboard's logical 1480×320 landscape geometry.

## Functional requirements

- The dashboard requests fullscreen presentation with a 1480×320 design geometry.
- An inset, fixed 64-pixel icon-only sidebar selects Overview, Systems, Projects, and Weather.
- Navigation controls expose selected and keyboard-focus states, provide accessible names, and use 48×48 logical-pixel touch targets.
- Pages cannot be changed by swiping.
- Overview shows the collapsible local-device card; Systems, Projects, and Weather show a translated heading and centered `Not implemented yet` empty state.
- Left and Right select the adjacent page without wrapping, and Home selects Overview.
- F5 focuses the Overview card chevron or the current empty state on other pages without changing pages or claiming that data was refreshed.
- Failure to create the root QML object terminates the dashboard with a nonzero status.
- The TTY launcher starts Cage without decorations and with VT switching enabled for manual recovery, explicitly selects Qt Wayland, and leaves startup diagnostics visible on stderr.
- The Cage session configures `HDMI-A-1` with the hardware-validated wlroots transform `270` before starting the dashboard.
- A missing `wlr-randr` command or failed output configuration prevents dashboard startup and reports a deterministic diagnostic on stderr.
- Background surfaces may extend beneath the panel's rounded corners, but text, focus indicators, controls, and other meaningful content must remain within a shared display-safe inset.

## Acceptance criteria

- A native release build succeeds on Raspberry Pi 5 with Qt 6.8 or newer.
- The dashboard starts through Cage from a local TTY, transforms the panel's native 320×1480 mode with wlroots transform `270`, and fills the resulting logical 1480×320 display without stretching or rectangular-edge clipping.
- Touching each sidebar entry selects its corresponding page.
- Left, Right, Home, and F5 behave as specified, including visible keyboard focus.
- Overview displays the local-device card; Systems, Projects, and Weather display `Not implemented yet`.
- Missing Cage, missing `wlr-randr`, failed output configuration, a missing dashboard executable, or root-QML initialization failure returns a nonzero status with a diagnostic visible on the TTY.
- The launcher preserves an explicitly supplied dashboard path, passes it through the Cage session helper, and propagates the session's exit status.

## Non-goals

- Live or fabricated telemetry, connectivity state, and refresh behavior.
- Swipe navigation.
- systemd units, autologin, installation packaging, or cross-compilation.
- A dependency on HoloNight libraries, QML modules, plugins, icons, configuration, or runtime resources.

## Verification

- Launch the dashboard with the offscreen Qt platform, verify it remains running after QML initialization, then terminate it cleanly.
- Validate the launcher and session helper with `sh -n` and deterministic fake Cage/`wlr-randr` tests covering rotation arguments, executable forwarding, exit propagation, and failure diagnostics.
- Run the focused startup test, followed by `task test` and `task check`.
- On a locally logged-in Raspberry Pi TTY, follow the release and Cage procedure in the README and manually verify the physical 320×1480 mode becomes logical 1480×320 geometry, fullscreen layout, five-point touch accuracy (four corners and center), touch navigation, keyboard navigation, focus indication, placeholders, hardware rendering, VT switching, console recovery, and clearance of meaningful content from the rounded corner cutouts.

## Validated hardware

The initial Raspberry Pi 5 validation passed on the Waveshare 11.9-inch 320×1480 HDMI touch display sold as product 25623. The USB touch controller identifies as `0712:000a`. The validated output transform, output association, calibration matrix, session requirements, and panel-safe-area findings are recorded in [the hardware validation report](../hardware-validation.md).
