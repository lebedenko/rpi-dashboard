# Release and production runbook

## One-time GitHub repository settings

Create a `main` ruleset with no bypass actors. Require pull requests with zero approvals, resolved
conversations, current branches, linear history, and the `dev`, `release`, `tidy`, `asan`, and
`ubsan` checks. Block force pushes and deletion. Enable squash merge only, automatic head-branch
deletion, and optionally auto-merge. Create a second `v*` tag ruleset that blocks updates and
deletions. Signed commits remain optional; release tags are signed annotated tags.

Set the default Actions token to read-only, disallow Actions-created pull requests, retain
artifacts for seven days, and require approval for first-time outside contributors. Allow only the
actions used by this repository; each workflow reference is pinned to a full commit SHA. Enable
Dependabot alerts and security updates. `.github/dependabot.yml` requests weekly updates for
GitHub Actions and the CI Docker base image.

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

Use: specification → short-lived branch → focused Conventional Commit changes → changelog entry →
pull request → required CI → squash merge. Prepare a release PR that removes remaining
`[Unreleased]` content, confirms both CMake versions, and runs focused release/deployment tests,
`task test`, `task check`, and `task ci`.

After merging, create and push the tag from updated `main`:

```sh
git tag -s -a v0.1.0 -m 'rpi-dashboard 0.1.0'
git push origin v0.1.0
```

Exercise the release workflow with a temporary non-release tag in a fork, or run
`scripts/release.sh archive 0.1.0 /tmp/rpi-dashboard-release` locally, before the production tag.
Never move a published release tag.

## Deploy and validate

Run **Deploy production** with version `0.1.0`, approve the environment gate, and retain its
deployment record. Confirm both installed binaries report `0.1.0`, then repeat the systemd,
journal, display, touch, keyboard, VT recovery, and SSH checks in `docs/hardware-validation.md`.
Backups under `/var/lib/rpi-dashboard/releases` are intentionally retained for the first release.
