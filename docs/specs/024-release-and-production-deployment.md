# Split release and Raspberry Pi production deployment

## Intent

Publish the immutable correction as `v0.1.1` from protected `main`. One release contains a
dashboard-only source archive and native static-musl daemon archives for x86_64 and aarch64, each
with its own checksum. Only the dashboard archive is deployed to the production Raspberry Pi.

## Release requirements

- `rpi-dashboard --version` and `dashboard-daemon --version` report the same version declared by
  their CMake projects.
- Only stable `vMAJOR.MINOR.PATCH` tags can publish. Both CMake versions and a dated Keep a
  Changelog entry must match, and the tagged commit must be an ancestor of `origin/main`.
- The release workflow creates deterministic `rpi-dashboard-VERSION.tar.gz`,
  `dashboard-daemon-VERSION-linux-x86_64.tar.gz`, and
  `dashboard-daemon-VERSION-linux-aarch64.tar.gz` assets with individual SHA-256 files. The
  dashboard manifest excludes `daemon/`; daemon packages contain exactly the documented four files.
- Daemons build and test natively in pinned Alpine/musl environments on matching GitHub-hosted
  architectures. JavaScript actions run on the Ubuntu host rather than inside Alpine so both x64
  and arm64 runners are supported. Packaging verifies version, ELF architecture, and absence of
  an interpreter.
- The release job alone receives `contents: write`; checkout credentials are not persisted.

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

- Deterministic shell tests reject malformed/mismatched versions, reproduce byte-identical source
  archives, verify checksums, enforce the staging allowlist, preserve configuration, exercise
  healthy activation, rollback, and failed-first-install recovery.
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
- Creating GitHub rulesets, secrets, tags, releases, or deployments from local test code. The
  signed annotated tag and protected production deployment remain operator actions after merge.
