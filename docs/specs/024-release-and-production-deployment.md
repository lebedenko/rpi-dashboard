# Build-before-tag release and Raspberry Pi production deployment

## Intent

Prepare explicit release metadata on a normal branch, review it through a pull request, and build
all release artifacts from the merged `main` commit before creating its GitHub Release and
lightweight tag. One release contains a dashboard-only source archive and native static-musl daemon
archives for x86_64 and aarch64, each with its own checksum. Only the dashboard archive is deployed
to the production Raspberry Pi.

## Release requirements

- The root `VERSION` file is the only version authority. Both CMake projects read it, and
  `rpi-dashboard --version` and `dashboard-daemon --version` report it.
- `task release:prepare VERSION=x.y.z` accepts only an explicit, unused, increasing stable version
  on a clean non-`main` branch. It updates `VERSION` and deterministically replaces `CHANGELOG.md`
  with a UTC-dated section generated from Conventional squash-merge subjects.
- Changelog generation starts after the latest successfully published stable GitHub Release, not
  the highest tag. It preserves published history and therefore ignores failed release-attempt tags.
- `feat` entries are Added; `fix` entries are Fixed; `perf` and `refactor` entries are Changed;
  `build`, `ci`, and dependency chores are Build and dependencies; and `docs` entries are
  Documentation. Breaking changes appear first. Tests, styles, generic chores, and release
  preparation commits are omitted. Squash PR numbers link to GitHub.
- `task release:check VERSION=x.y.z` validates metadata and deterministic regeneration before
  publication. `task release:publish VERSION=x.y.z` requires clean synchronized `main`, successful
  CI for its exact SHA, and no conflicting release or tag, then dispatches and watches the release.
- The release workflow creates deterministic `rpi-dashboard-VERSION.tar.gz`,
  `dashboard-daemon-VERSION-linux-x86_64.tar.gz`, and
  `dashboard-daemon-VERSION-linux-aarch64.tar.gz` assets with individual SHA-256 files. The
  dashboard manifest excludes `daemon/`; daemon packages contain exactly the documented four files.
- Daemons build and test natively in pinned Alpine/musl environments on matching GitHub-hosted
  architectures. JavaScript actions run on the Ubuntu host rather than inside Alpine so both x64
  and arm64 runners are supported. Packaging verifies version, ELF architecture, and absence of
  an interpreter.
- The dispatch-only release workflow is serialized, validates the requested version and exact
  `main` SHA, and builds every artifact before its final job receives `contents: write`. Checkout
  credentials are not persisted. The final job uses the committed changelog section as the release
  body and atomically creates the lightweight tag with `gh release create --target "$GITHUB_SHA"`.
- An interrupted final publication may leave a draft for the same version and SHA; retry resumes
  that draft. Existing tags or releases pointing elsewhere are never moved, overwritten, or deleted.

## Deployment requirements

- Deployment is manual, serialized under `raspberry-pi-production`, never cancels an active run,
  and accepts only an existing non-draft, non-prerelease stable release.
- The protected environment supplies Tailscale federated client ID and audience, the Pi MagicDNS
  hostname and user, a dedicated SSH key, and a pinned OpenSSH `known_hosts` line. The ephemeral
  `tag:ci` node may reach only TCP/22 on the tagged production Pi.
- The archive checksum is verified on the runner and again on the Pi. The Pi builds and tests the
  Debug preset unprivileged, then builds and stages the Release preset unprivileged.
- The root-owned activation helper accepts only the documented runtime allowlist. It preserves
  `/usr/local/etc/xdg/rpi-dashboard/config.toml` and systemd encrypted credentials, snapshots the
  previous runtime, reloads systemd, and restarts the kiosk.
- Health requires `active/running`, one stable nonzero main PID, and no restart-count increase for
  15 seconds. Failure restores the previous runtime. With no prior runtime it stops the kiosk and
  restores `getty@tty1`.
- Deployment records and backups are retained without automatic deletion for the initial release.
- Repository settings require pull requests with resolved conversations, current required checks,
  linear squash history, no bypass, and immutable `v*` tags. Actions tokens default to read-only;
  only the release job receives write access.

## Observable acceptance criteria

- Deterministic shell tests reject malformed, reused, non-increasing, and mismatched versions;
  verify changelog categories, omissions, breaking-first ordering, links, dates, baseline selection,
  and regeneration; exercise publication guards and retry conflicts; reproduce byte-identical
  archives; and verify checksums, staging, activation, rollback, and failed-first-install recovery.
- CI runs `dev`, `release`, `tidy`, `asan`, and `ubsan` as separately required checks.
- CI also runs a separately required native daemon build and test check.
- Both release daemon matrix entries run to completion independently, and each uploads its assets
  after the pinned Alpine build succeeds.
- A production workflow run records the protected environment and requested version, and the
  deployed binaries report that version.

## Non-goals

- Cross-compiling daemon binaries, deploying daemon packages, or including daemon files in the
  dashboard source/stage.
- A persistent self-hosted runner, automatic production deployment, static public SSH addressing,
  automatic backup deletion, or committing credentials and hardware-specific configuration.
- Inferring a version from commits; automating commits, branches, pull requests, merges, or
  production deployment; creating GitHub rulesets or secrets; or moving/deleting failed `v0.1.1`
  and `v0.1.2` tags. Version choice and production deployment remain explicit operator actions.
