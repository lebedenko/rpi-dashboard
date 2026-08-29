#!/bin/sh
set -eu
fail() { echo "dashboard-daemon install: $*" >&2; exit 1; }
[ "${DASHBOARD_DAEMON_ALLOW_UNPRIVILEGED:-0}" = 1 ] || [ "$(id -u)" -eq 0 ] || fail "must run as root"
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
package_arch=${DASHBOARD_DAEMON_PACKAGE_ARCH:-@PACKAGE_ARCH@}
host_arch=${DASHBOARD_DAEMON_HOST_ARCH:-$(uname -m)}
if [ "$package_arch" = '@PACKAGE_ARCH@' ]; then package_arch=$host_arch; fi
[ "$package_arch" = x86_64 ] || [ "$package_arch" = aarch64 ] || fail "unsupported package architecture"
[ "$host_arch" = "$package_arch" ] || fail "package architecture $package_arch is incompatible with $host_arch"
systemctl_command=${DASHBOARD_DAEMON_SYSTEMCTL:-systemctl}
command -v "$systemctl_command" >/dev/null 2>&1 || fail "systemd is required"
root=${DASHBOARD_DAEMON_INSTALL_ROOT:-}
binary="$root/usr/local/bin/dashboard-daemon"; unit="$root/usr/local/lib/systemd/system/dashboard-daemon.service"; config="$root/etc/xdg/dashboard-daemon/config.toml"
candidate_config="$src/config.toml"; [ ! -f "$config" ] || candidate_config=$config
"$src/dashboard-daemon" --config "$candidate_config" --check-config >/dev/null || fail "configuration is invalid"
had_binary=0; had_unit=0; was_enabled=0; was_active=0
[ ! -f "$binary" ] || had_binary=1; [ ! -f "$unit" ] || had_unit=1
"$systemctl_command" is-enabled --quiet dashboard-daemon.service 2>/dev/null && was_enabled=1 || :
"$systemctl_command" is-active --quiet dashboard-daemon.service 2>/dev/null && was_active=1 || :
backup=$(mktemp -d "${TMPDIR:-/tmp}/dashboard-daemon-install-XXXXXX"); trap 'rm -rf -- "$backup"' EXIT HUP INT TERM
[ "$had_binary" -eq 0 ] || cp -p "$binary" "$backup/dashboard-daemon"
[ "$had_unit" -eq 0 ] || cp -p "$unit" "$backup/dashboard-daemon.service"
install -d -m 0755 "$(dirname "$binary")" "$(dirname "$unit")" "$(dirname "$config")"
[ -f "$config" ] || { install -m 0644 "$src/config.toml" "$config.new"; mv "$config.new" "$config"; }
install -m 0755 "$src/dashboard-daemon" "$binary.new"; mv "$binary.new" "$binary"
install -m 0644 "$src/dashboard-daemon.service" "$unit.new"; mv "$unit.new" "$unit"
"$systemctl_command" daemon-reload
if [ "$had_binary" -eq 1 ]; then "$systemctl_command" enable dashboard-daemon.service; "$systemctl_command" restart dashboard-daemon.service
else "$systemctl_command" enable --now dashboard-daemon.service; fi
healthy=1; pid=; restarts=; i=0
while [ "$i" -lt 5 ]; do
  active=$("$systemctl_command" is-active dashboard-daemon.service 2>/dev/null || :)
  current_pid=$("$systemctl_command" show dashboard-daemon.service -p MainPID --value 2>/dev/null || :)
  current_restarts=$("$systemctl_command" show dashboard-daemon.service -p NRestarts --value 2>/dev/null || :)
  [ "$active" = active ] && [ "${current_pid:-0}" -gt 0 ] 2>/dev/null || healthy=0
  if [ "$i" -eq 0 ]; then pid=$current_pid; restarts=$current_restarts; else [ "$pid" = "$current_pid" ] && [ "$restarts" = "$current_restarts" ] || healthy=0; fi
  [ "$healthy" -eq 1 ] || break; sleep "${DASHBOARD_DAEMON_HEALTH_INTERVAL:-1}"; i=$((i + 1))
done
[ "$healthy" -eq 1 ] && { echo "dashboard-daemon install: installed and active"; exit 0; }
"$systemctl_command" stop dashboard-daemon.service >/dev/null 2>&1 || :
if [ "$had_binary" -eq 1 ]; then cp -p "$backup/dashboard-daemon" "$binary"; else rm -f "$binary"; fi
if [ "$had_unit" -eq 1 ]; then cp -p "$backup/dashboard-daemon.service" "$unit"; else rm -f "$unit"; fi
"$systemctl_command" daemon-reload >/dev/null 2>&1 || :
if [ "$was_enabled" -eq 1 ]; then "$systemctl_command" enable dashboard-daemon.service >/dev/null 2>&1 || :; else "$systemctl_command" disable dashboard-daemon.service >/dev/null 2>&1 || :; fi
[ "$was_active" -eq 0 ] || "$systemctl_command" start dashboard-daemon.service >/dev/null 2>&1 || :
fail "service did not remain healthy; previous runtime state restored"
