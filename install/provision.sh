#!/bin/sh

set -eu

account=dashboard
expected_home=/home/dashboard
expected_shell=/usr/sbin/nologin
model_file=${HOLONIGHT_MODEL_FILE:-/proc/device-tree/model}

fail() {
  echo "holonight-provision: $*" >&2
  exit 1
}

[ "${HOLONIGHT_ALLOW_UNPRIVILEGED:-0}" = 1 ] || [ "$(id -u)" -eq 0 ] || fail "must be run as root"
[ "$(uname -m)" = aarch64 ] || fail "requires aarch64"
[ -r "$model_file" ] || fail "cannot identify Raspberry Pi model"
model=$(tr -d '\000' <"$model_file")
case "$model" in
  *"Raspberry Pi 5"*) ;;
  *) fail "requires a Raspberry Pi 5" ;;
esac
[ -d /run/systemd/system ] || [ "${HOLONIGHT_SYSTEMD_AVAILABLE:-0}" = 1 ] || fail "systemd is not running"
command -v cage >/dev/null 2>&1 || fail "cage is not installed or not available in PATH"
command -v wlr-randr >/dev/null 2>&1 || fail "wlr-randr is not installed or not available in PATH"

if getent passwd "$account" >/dev/null 2>&1; then
  entry=$(getent passwd "$account")
  account_gid=$(printf '%s\n' "$entry" | cut -d: -f4)
  home=$(printf '%s\n' "$entry" | cut -d: -f6)
  shell=$(printf '%s\n' "$entry" | cut -d: -f7)
  primary_group=$(getent group "$account_gid" | cut -d: -f1)
  groups=$(id -Gn "$account")
  password_state=$(passwd -S "$account" | awk '{print $2}')
  [ "$home" = "$expected_home" ] || fail "existing dashboard account has incompatible home: $home"
  [ "$shell" = "$expected_shell" ] || fail "existing dashboard account has incompatible shell: $shell"
  [ "$primary_group" = "$account" ] || fail "existing dashboard account has incompatible primary group"
  [ "$groups" = "$account" ] || fail "existing dashboard account has supplementary groups: $groups"
  [ "$password_state" = L ] || fail "existing dashboard account password is not locked"
else
  useradd --create-home --user-group --shell "$expected_shell" "$account"
  passwd --lock "$account" >/dev/null
fi

systemctl daemon-reload
systemctl enable holonight-dashboard.service
echo "holonight-provision: installed and enabled; reboot or start the service explicitly"
