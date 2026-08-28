#!/bin/sh

set -eu

stage=${1:-}
[ -n "$stage" ] && [ -d "$stage" ] || {
    echo "activate-release: staged install root is required" >&2
    exit 2
}

if [ "$(id -u)" -ne 0 ] && [ "${RPI_DASHBOARD_ALLOW_UNPRIVILEGED:-0}" != 1 ]; then
    echo "activate-release: must run as root" >&2
    exit 1
fi

prefix=${RPI_DASHBOARD_PREFIX:-/usr/local}
backup_root=${RPI_DASHBOARD_BACKUP_ROOT:-/var/lib/rpi-dashboard/releases}
systemctl_command=${SYSTEMCTL_COMMAND:-systemctl}
health_seconds=${RPI_DASHBOARD_HEALTH_SECONDS:-15}
stamp=${RPI_DASHBOARD_RELEASE_ID:-$(date -u +%Y%m%dT%H%M%SZ)}
backup=$backup_root/$stamp
manifest=$(mktemp "${TMPDIR:-/tmp}/rpi-dashboard-stage-XXXXXX")
trap 'rm -f -- "$manifest"' EXIT HUP INT TERM

(cd "$stage" && find . -type f -o -type l) | LC_ALL=C sort >"$manifest"
cat >"$manifest.expected" <<'EOF'
./usr/local/bin/rpi-dashboard
./usr/local/etc/xdg/rpi-dashboard/config.toml
./usr/local/lib/systemd/system/rpi-dashboard.service
./usr/local/libexec/rpi-dashboard/activate-release.sh
./usr/local/libexec/rpi-dashboard/provision.sh
./usr/local/libexec/rpi-dashboard/run-dashboard-session.sh
./usr/local/libexec/rpi-dashboard/run-kiosk.sh
EOF
trap 'rm -f -- "$manifest" "$manifest.expected"' EXIT HUP INT TERM
cmp "$manifest.expected" "$manifest" || {
    echo "activate-release: staged files do not match the runtime allowlist" >&2
    exit 1
}

had_install=0
if [ -x "$prefix/bin/rpi-dashboard" ]; then
    had_install=1
    mkdir -p "$backup"
    for relative in bin/rpi-dashboard lib/systemd/system/rpi-dashboard.service \
        libexec/rpi-dashboard/activate-release.sh libexec/rpi-dashboard/provision.sh \
        libexec/rpi-dashboard/run-dashboard-session.sh libexec/rpi-dashboard/run-kiosk.sh; do
        if [ -e "$prefix/$relative" ]; then
            mkdir -p "$backup/$(dirname "$relative")"
            cp -p "$prefix/$relative" "$backup/$relative"
        fi
    done
fi

install -d "$prefix/bin" "$prefix/lib/systemd/system" "$prefix/libexec/rpi-dashboard"
install -m 0755 "$stage/usr/local/bin/rpi-dashboard" "$prefix/bin/rpi-dashboard"
install -m 0644 "$stage/usr/local/lib/systemd/system/rpi-dashboard.service" \
    "$prefix/lib/systemd/system/rpi-dashboard.service"
for helper in activate-release.sh provision.sh run-dashboard-session.sh run-kiosk.sh; do
    install -m 0755 "$stage/usr/local/libexec/rpi-dashboard/$helper" "$prefix/libexec/rpi-dashboard/$helper"
done
config=$prefix/etc/xdg/rpi-dashboard/config.toml
if [ ! -e "$config" ]; then
    install -d "$(dirname "$config")"
    install -m 0644 "$stage/usr/local/etc/xdg/rpi-dashboard/config.toml" "$config"
fi

rollback() {
    if [ "$had_install" -eq 1 ]; then
        for relative in bin/rpi-dashboard lib/systemd/system/rpi-dashboard.service \
            libexec/rpi-dashboard/activate-release.sh libexec/rpi-dashboard/provision.sh \
            libexec/rpi-dashboard/run-dashboard-session.sh libexec/rpi-dashboard/run-kiosk.sh; do
            [ ! -e "$backup/$relative" ] || install -m "$(stat -c %a "$backup/$relative")" \
                "$backup/$relative" "$prefix/$relative"
        done
        "$systemctl_command" daemon-reload
        "$systemctl_command" restart rpi-dashboard.service
    else
        "$systemctl_command" stop rpi-dashboard.service || true
        "$systemctl_command" enable --now getty@tty1.service || true
    fi
}

"$systemctl_command" daemon-reload
"$systemctl_command" restart rpi-dashboard.service
initial_restarts=$("$systemctl_command" show rpi-dashboard.service --property=NRestarts --value)
initial_pid=$("$systemctl_command" show rpi-dashboard.service --property=MainPID --value)
case "$initial_pid" in ''|0|*[!0-9]*) rollback; exit 1 ;; esac

elapsed=0
while [ "$elapsed" -lt "$health_seconds" ]; do
    state=$("$systemctl_command" show rpi-dashboard.service --property=ActiveState --value)
    substate=$("$systemctl_command" show rpi-dashboard.service --property=SubState --value)
    pid=$("$systemctl_command" show rpi-dashboard.service --property=MainPID --value)
    restarts=$("$systemctl_command" show rpi-dashboard.service --property=NRestarts --value)
    if [ "$state:$substate" != active:running ] || [ "$pid" != "$initial_pid" ] || \
        [ "$restarts" != "$initial_restarts" ]; then
        rollback
        exit 1
    fi
    sleep 1
    elapsed=$((elapsed + 1))
done

mkdir -p "$backup_root"
printf '%s\n' "$stamp" >"$backup_root/current-release"
