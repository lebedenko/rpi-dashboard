# Dashboard configuration

## Intent

Provide one installed, commented TOML file that is the authoritative inventory of dashboard
settings on systems where no manual page is installed.

## Configuration

The dashboard discovers `rpi-dashboard/config.toml` through the XDG configuration directories;
`--config PATH` overrides discovery. The schema contains display mode and geometry, the GitHub
owner, the remote telemetry listener, credential-file paths, and weather provider/location.
Command-line options override corresponding file values. Credential-file environment variables
override file paths, while raw token environment variables remain development-only fallbacks.

The installed template documents every supported key, its default, valid alternatives, precedence,
and credential setup without containing secret values. Mutually exclusive weather location choices
remain commented examples rather than simultaneously active keys.

## Acceptance criteria

- The shipped template parses successfully and contains every supported TOML key.
- Display, projects, telemetry, credentials, and weather settings parse deterministically.
- Invalid dimensions, ports, IPv4 addresses, types, and weather values fail an explicit config load.
- Existing command-line and environment interfaces remain compatible and take precedence.
- Installation creates the complete template when absent and preserves existing configuration on
  upgrades.

## Non-goals

- Storing API keys or GitHub tokens directly in TOML.
- Configuring kiosk hardware, systemd policy, or the standalone telemetry daemon from this file.
- Supporting undocumented keys as a compatibility contract.
