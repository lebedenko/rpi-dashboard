# Preserve dashboard restarts on tty1

## Intent

Keep tty1 assigned to the dashboard across an explicit systemd restart without changing the
existing clean-exit behavior that restores the tty1 getty.

## Functional requirements

- The packaged `rpi-dashboard.service` conflicts with `getty@tty1.service` while the dashboard is
  active.
- A successful dashboard exit starts `getty@tty1.service` so Ctrl+Q and an explicit stop return
  tty1 to a login prompt.
- The successful-exit job uses `OnSuccessJobMode=fail`. During an explicit dashboard restart, a
  queued getty start that conflicts with the already queued dashboard start must fail instead of
  replacing the dashboard restart transaction.
- Restarting the dashboard after a configuration change leaves the service active and loads the
  changed configuration.

## Observable acceptance criteria

- The deterministic install contract test requires the installed unit to contain
  `Conflicts=getty@tty1.service`, `OnSuccess=getty@tty1.service`, and
  `OnSuccessJobMode=fail`.
- `systemd-analyze verify` accepts the packaged unit.
- On the target Raspberry Pi, `systemctl restart rpi-dashboard.service` leaves the dashboard
  active and applies changed configuration.
- On the target Raspberry Pi, Ctrl+Q cleanly exits the dashboard and activates
  `getty@tty1.service`.
- On the target Raspberry Pi, an explicit stop restores the getty, and a later start returns tty1
  to the dashboard.

## Non-goals

- Changing launcher, activation, application, or configuration-schema behavior.
- Changing the dashboard restart policy or public APIs.
- Replacing target-hardware validation of the complete systemd transaction and tty ownership with
  a repository-only simulation.
