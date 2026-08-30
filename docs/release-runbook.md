# Release and production runbook

## One-time GitHub repository settings

Create a `main` ruleset with no bypass actors. Require pull requests with zero approvals, resolved
conversations, current branches, linear history, and the `dev`, `release`, `tidy`, `asan`, and
`ubsan`, `daemon`, and `conventional-title` checks. Block force pushes and deletion. Enable squash
merge only, automatic head-branch deletion, and optionally auto-merge. Create a second `v*` tag
ruleset that blocks updates and deletions. Signed commits remain optional; the release workflow
creates lightweight tags only after every artifact has built successfully.

Set the default Actions token to read-only, disallow Actions-created pull requests, retain
artifacts for seven days, and require approval for first-time outside contributors. Allow only the
actions used by this repository; each workflow reference is pinned to a full commit SHA. Enable
Dependabot alerts and security updates. `.github/dependabot.yml` requests weekly updates for
GitHub Actions and the CI Docker base image.

The Actions allowlist includes pinned `actions/checkout`, `actions/upload-artifact`,
`actions/download-artifact`, and `tailscale/github-action`. The daemon release jobs use the pinned
multi-architecture Alpine image digest recorded in the workflow.

Create the protected `raspberry-pi-production` environment, restrict it to protected `main`, require
manual approval while allowing self-review, and disable administrator bypass. Add:

- secrets `TS_OAUTH_CLIENT_ID`, `TS_AUDIENCE`, `PI_SSH_PRIVATE_KEY`, and `PI_SSH_HOST_KEY`;
- variables `PI_HOST` (MagicDNS name) and `PI_SSH_USER`.

The Tailscale federated identity needs writable `auth_keys` scope for `tag:ci`. Tailnet policy must
permit `tag:ci` to reach only the production Pi tag on TCP/22.

## One-time Pi bootstrap

Install Tailscale, Qt 6.8+, CMake, Ninja, a C++23 compiler, Task, and project build dependencies.
Tag the Pi as production. Create an unprivileged deployment user with the dedicated SSH public key.
Install `activate-release.sh` root-owned at `/usr/local/libexec/rpi-dashboard/activate-release.sh`,
then grant that user passwordless sudo for that exact helper only. Do not expose SSH or sudo outside
the tailnet.

## Development and release

Use: specification → short-lived branch → focused Conventional Commit changes → pull request →
required CI → squash merge. PR titles must be Conventional Commit subjects because squash titles
become changelog entries. Tests, styles, generic chores, and `chore(release)` commits are omitted;
use a supported release-note type for user-visible work.

On a clean short-lived branch, explicitly choose a greater unused stable version and generate the
release metadata:

```sh
task release:prepare VERSION=x.y.z
```

Review the root `VERSION` and generated `CHANGELOG.md`. Changelog generation uses the latest
successfully published stable GitHub Release as its baseline, so failed tag attempts do not hide
commits. Commit the two files as `chore(release): prepare x.y.z`, open a normal PR, and run:

```sh
task release:check VERSION=x.y.z
task test
task check
task ci
```

After merge, update a clean local `main` so it exactly matches `origin/main`, then publish:

```sh
task release:publish VERSION=x.y.z
```

The command requires successful CI for that exact SHA, dispatches the serialized release workflow,
and watches it to completion. Validation or build failure creates no tag or release. If final
publication is interrupted, rerunning may resume only a draft for the same version and SHA. Never
move, overwrite, or delete an existing tag or published release.

Confirm that the release has three archives and three adjacent checksum files. Verify each daemon
archive's four-file prefixed manifest, executable version, ELF architecture, and static linkage.
Operators edit the unpacked `config.toml` before `sudo ./install.sh`; upgrades retain host
configuration and roll back runtime files and service state after a failed health check.

## Deploy and validate

Run **Deploy production** with the published version, approve the environment gate, and retain its
deployment record. The workflow transfers only the dashboard archive and checksum. Confirm the
dashboard reports the selected version, then repeat the systemd,
journal, display, touch, keyboard, VT recovery, and SSH checks in `docs/hardware-validation.md`.
Backups under `/var/lib/rpi-dashboard/releases` are intentionally retained for the first release.
