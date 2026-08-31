# Raspberry Pi 5 Hardware Validation

## Validation record

Validation passed on 2026-08-24 using a Raspberry Pi 5 and a display sold as “11.9inch IPS Display, 320×1480, Toughened Glass Panel, HDMI, Touch” with product identifier 25623. The panel exposes a native 320×1480 portrait mode on `HDMI-A-1`. Its USB touchscreen identifies as Waveshare `0712:000a` and supports ten contacts.

The following checks passed:

- Debug, clang-tidy, and Release builds.
- All automated tests, including the deterministic Cage and `wlr-randr` launcher test.
- Resolution of every dynamic library used by the Release dashboard.
- Hardware-accelerated Qt Quick rendering under Cage.
- Logical 1480×320 fullscreen output without stretching or rectangular-edge clipping.
- Five-point touch mapping at all four corners and the center.
- Touch selection of Overview, Systems, Projects, and Weather.
- Left, Right, Home, and F5 keyboard behavior.
- VT switching, console recovery, and SSH recovery.
- Non-root Cage operation from an active local logind TTY session.

The Qt build emits Debian dynamic-QML-plugin import warnings even though the application builds, its startup test passes, and it runs without QML errors. These remain packaging diagnostics. Do not suppress them with private Qt development packages, private targets, or private headers.

## Display and console rotation

The validated Wayland output command is:

```sh
wlr-randr --output HDMI-A-1 --transform 270
```

Transform `90` produced an upside-down landscape image on this panel. `wlr-randr` is a Wayland client and reports `failed to connect to display` when invoked outside the active Cage session; the session helper therefore runs it after Cage starts and before the dashboard.

The kernel command-line option `fbcon=rotate:1` is retained so boot messages and local virtual consoles are upright. Framebuffer-console rotation and the wlroots output transform configure separate display layers and do not replace one another.

## Seat and launch requirements

Run the manual launcher as the logged-in user from an active physical TTY. Cage must obtain the VT, DRM, and input devices through PAM/logind. Running it from SSH or a session that does not own `seat0` can produce errors such as `Could not open target tty: Permission denied`.

Do not run the dashboard with `sudo`, loosen device-node permissions, or add the dashboard account permanently to `video`, `render`, `input`, or `tty`. SSH remains the administrative recovery path.

Useful diagnostics are:

```sh
loginctl session-status
loginctl seat-status seat0
```

The active session must be a local TTY session attached to `seat0` with an `XDG_RUNTIME_DIR` such as `/run/user/<uid>`.

## Installed kiosk acceptance

After `task install`, reboot and confirm that tty1 launches the dashboard directly. The installer
enables the unit but intentionally does not start it in the current session. Verify the runtime
identity, seat, and per-user runtime directory with:

```sh
systemctl status rpi-dashboard.service
journalctl -u rpi-dashboard.service -b
loginctl session-status
loginctl seat-status seat0
sudo -u dashboard test -d /run/user/$(id -u dashboard)
```

The service process must run as `dashboard`, the local session must own tty1 on `seat0`, and the
runtime-directory check must succeed. When the encrypted `github-token` credential is installed,
confirm that private GitHub data loads without credential text appearing in the journal. Change a
visible configuration value, run `sudo systemctl restart rpi-dashboard.service`, and confirm that
the dashboard remains active and displays the changed value. Press Ctrl+Q and confirm that the
clean exit starts `getty@tty1.service` and returns to a login prompt. Start the dashboard again,
run `sudo systemctl stop rpi-dashboard.service`, and confirm that the getty returns; a later
`sudo systemctl start rpi-dashboard.service` must return tty1 to the dashboard.
Finally repeat the display transform, five-point touch, keyboard navigation, VT switching, and SSH
recovery checks recorded above.

## Touchscreen configuration

Raw five-point testing showed that the controller reports portrait-native coordinates while the panel is physically used in landscape. The observed mapping was:

| Physical point | Approximate raw point |
| --- | --- |
| Left top | `(95, 1)` |
| Left bottom | `(7, 1)` |
| Right bottom | `(11, 97)` |
| Right top | `(88, 99)` |
| Center | `(50, 52)` |

The required conversion is `logical X = raw Y` and `logical Y = 1 - raw X`, represented by calibration matrix `0 1 0 -1 0 1`.

Install this device-specific rule as `/etc/udev/rules.d/90-waveshare-touchscreen.rules`:

```udev
ACTION=="add|change", SUBSYSTEM=="input", KERNEL=="event*", \
ENV{ID_INPUT_TOUCHSCREEN}=="1", \
ENV{ID_VENDOR_ID}=="0712", ENV{ID_MODEL_ID}=="000a", \
ENV{WL_OUTPUT}="HDMI-A-1", \
ENV{LIBINPUT_CALIBRATION_MATRIX}="0 1 0 -1 0 1"
```

Reload the rule and restart the entire Cage session:

```sh
sudo udevadm control --reload-rules
sudo udevadm trigger --subsystem-match=input
```

Verify the effective properties before launching Cage:

```sh
udevadm info --query=property --name=/dev/input/event5 \
    | grep -E 'ID_INPUT_TOUCHSCREEN|WL_OUTPUT|LIBINPUT_CALIBRATION_MATRIX'
```

The event number is diagnostic only and may change across boots; the udev rule intentionally matches the stable USB vendor and product IDs instead.

## Rounded-corner safe area

The glass viewport has rounded corners that physically obscure pixels at all four corners. This is expected panel geometry, not rectangular framebuffer clipping.

The page background may render edge to edge. Text, icons, focus borders, touch-control visuals, bordered surfaces, and other meaningful content must remain inside a shared safe content rectangle. The measured inset is exposed as the central `Theme.displaySafeInset` layout token rather than being repeated as component-specific margins. Touch hit areas may extend into the inset provided their visible and meaningful content remains unobscured.

The measurement procedure and evidence requirements are defined in [Rounded-Corner Display Safe Area](specs/002-rounded-corner-safe-area.md). Calibration was performed on 2026-08-24 using one straight-on and four close-corner 4080×3072 photographs. A 9 px equal X/Y outline was completely visible at every corner; at 8 px, one pixel at the limiting magenta corner edge was obscured. The measured safe inset is therefore 9 px. Applying the specified 1 px guard produces `Theme.displaySafeInset = 10`.

Photo-derived corner estimates are approximately 20 px top-left, 22 px top-right, 26 px bottom-right, and 23 px bottom-left, with approximately 2.5–3 px fit residual and ±4 px radius uncertainty. Effective horizontal/vertical estimates are 17/22, 22/23, 27/25, and 20/25 px respectively. Perspective, display-pixel moiré, and glass reflections make these radii less precise than the directly observed inset; layout clearance uses the inset only.

The calibration outline passed at all four corners. Final-dashboard inspection of every page, focus border, and navigation method remains pending after applying the token.
