# Raspberry Pi kiosk installation

## Intent

Provide a repeatable system-wide installation for a dedicated Raspberry Pi 5 running 64-bit
Raspberry Pi OS Lite. The installation builds the dashboard as the invoking developer, installs
only the runtime files under `/usr/local`, and provisions a locked-down tty1 kiosk account and
service without disrupting the current console session.

## Functional requirements

- `task install` configures and builds the Release preset without privilege escalation. It uses
  `sudo` only to install the built files and run the provisioning helper.
- A dashboard source package exposes the same lifecycle through `./install.sh`: configure, build,
  and test as the invoking user, then use `sudo` only for CMake installation and provisioning.
- The installed runtime consists of `/usr/local/bin/rpi-dashboard`, launcher and provisioning
  and fixed release-activation helpers in `/usr/local/libexec/rpi-dashboard/`, and
  `/usr/local/lib/systemd/system/rpi-dashboard.service`.
- Installation creates the dashboard configuration from the shipped defaults when it is absent.
  When it already exists, installation preserves its contents and appends newly shipped default
  sections exactly once instead of replacing the file.
- Provisioning fails unless the host is an aarch64 Raspberry Pi 5 running systemd with `cage` and
  `wlr-randr` available. It does not install packages.
- Provisioning creates `dashboard` when absent, with `/home/dashboard`, its own primary group,
  `/usr/sbin/nologin`, and a locked password. It grants no supplementary device or sudo groups.
- An existing account is left unchanged only when its home, shell, primary group, supplementary
  groups, and password-lock state match the supported configuration. Any mismatch is reported and
  provisioning stops.
- Provisioning reloads systemd and enables `rpi-dashboard.service`; it never starts or
  restarts the service.
- The service replaces `getty@tty1`, runs as `dashboard`, opens a PAM/logind session on tty1, lets
  Cage manage VT switching, restarts only after failure, and sends diagnostics to both the journal
  and console.

## GitHub credential handling

- `GITHUB_TOKEN_FILE` names an optional credential file. When configured it takes precedence over
  `GITHUB_TOKEN`; trailing CR/LF bytes are removed before use.
- A missing, unreadable, or empty configured file produces a generic diagnostic and anonymous
  GitHub access. Diagnostics never contain credential contents or fall back to `GITHUB_TOKEN` in
  this case.
- Without `GITHUB_TOKEN_FILE`, `GITHUB_TOKEN` remains supported for local development.
- The service requests the optional encrypted systemd credential `github-token` and sets
  `GITHUB_TOKEN_FILE` to its runtime credential path.

For production, create and populate the encrypted credential store without exposing the token in
shell history or an environment file:

```sh
sudo install -d -m 0700 /etc/credstore.encrypted
sudo -v
systemd-ask-password 'GitHub token' | \
  sudo systemd-creds encrypt --name=github-token - /etc/credstore.encrypted/github-token
sudo systemctl restart rpi-dashboard.service
```

Systemd recommends credentials rather than environment variables for secrets; see the
[systemd credential guidance](https://manpages.debian.org/testing/systemd/systemd.exec.5.en.html).

## Failure and security behavior

- Failed prerequisite or account validation leaves service enablement untouched and exits with a
  non-zero status. Re-running after a successful installation is safe and does not recreate or
  modify the account.
- The personal access token is never installed by CMake, stored in `.profile`, placed in shell
  history, or configured through `EnvironmentFile`.
- Runtime launch failures are visible on tty1 and through `journalctl -u rpi-dashboard`.

## Observable acceptance criteria

- A temporary `DESTDIR` installation contains every documented path with executable helpers and
  service contents intact, and contains no daemon binary, unit, configuration, or package file.
- A repeated temporary `DESTDIR` installation preserves existing dashboard configuration, adds the
  shipped weather defaults once, and does not duplicate them.
- A deterministic fake-host test covers initial account creation, an idempotent valid-account run,
  systemd daemon reload, enablement, and the absence of any start/restart action.
- Credential tests cover environment fallback, file precedence, CR/LF removal, missing and empty
  files, and verify that diagnostics do not reveal token text.
- After reboot on the target Pi, tty1 launches Cage and the dashboard as `dashboard`; `loginctl`
  shows a local seat session and `/run/user/<uid>` exists. Authenticated GitHub data loads when the
  encrypted credential is installed, Ctrl+Q starts the tty1 getty and returns to a login prompt,
  and the existing physical display and touch checks still pass.

## Hardware-specific post-install steps

Framebuffer-console rotation and touchscreen calibration remain manual, device-specific steps in
[`docs/hardware-validation.md`](../hardware-validation.md). They are not installed automatically.

## Non-goals

- Installing Qt, build tools, Cage, `wlr-randr`, or any APT packages.
- Supporting non-aarch64 systems, Raspberry Pi models other than Pi 5, configurable install
  prefixes, or account names other than `dashboard` for production.
- Installing the remote telemetry agent.
- Building, testing, staging, or packaging `dashboard-daemon` as part of the dashboard graph.
- Semantically merging individual keys into an existing configuration section.
- Starting the kiosk during installation or configuring display/touch hardware automatically.
- Downloading releases or granting general passwordless sudo; production deployment is specified
  separately and may invoke only the root-owned activation helper.
