#!/bin/sh

set -eu

activation=$1
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/rpi-dashboard-activation-test-XXXXXX")
trap 'rm -rf -- "$test_dir"' EXIT HUP INT TERM
stage=$test_dir/stage
prefix=$test_dir/prefix
state=$test_dir/state
mkdir -p "$stage/usr/local/bin" "$stage/usr/local/etc/xdg/rpi-dashboard" \
    "$stage/usr/local/lib/systemd/system" "$stage/usr/local/libexec/rpi-dashboard" "$state"
printf '%s\n' new >"$stage/usr/local/bin/rpi-dashboard"
printf '%s\n' defaults >"$stage/usr/local/etc/xdg/rpi-dashboard/config.toml"
printf '%s\n' service >"$stage/usr/local/lib/systemd/system/rpi-dashboard.service"
for helper in activate-release.sh provision.sh run-dashboard-session.sh run-kiosk.sh; do
    cp "$activation" "$stage/usr/local/libexec/rpi-dashboard/$helper"
done
chmod +x "$stage/usr/local/bin/rpi-dashboard" "$stage/usr/local/libexec/rpi-dashboard/"*

fake_systemctl=$test_dir/systemctl
cat >"$fake_systemctl" <<'EOF'
#!/bin/sh
printf '%s\n' "$*" >>"$TEST_STATE/systemctl.log"
case "$*" in
  *--property=ActiveState*) [ -f "$TEST_STATE/fail" ] && echo failed || echo active ;;
  *--property=SubState*) echo running ;;
  *--property=MainPID*) echo 4321 ;;
  *--property=NRestarts*) echo 0 ;;
esac
EOF
chmod +x "$fake_systemctl"

run_activation() {
    TEST_STATE="$state" SYSTEMCTL_COMMAND="$fake_systemctl" RPI_DASHBOARD_ALLOW_UNPRIVILEGED=1 \
        RPI_DASHBOARD_PREFIX="$prefix" RPI_DASHBOARD_BACKUP_ROOT="$test_dir/backups" \
        RPI_DASHBOARD_HEALTH_SECONDS=1 RPI_DASHBOARD_RELEASE_ID="$1" "$activation" "$stage"
}

run_activation first
grep -Fqx new "$prefix/bin/rpi-dashboard"
printf '%s\n' private-config >"$prefix/etc/xdg/rpi-dashboard/config.toml"
printf '%s\n' old >"$prefix/bin/rpi-dashboard"
printf '%s\n' replacement >"$stage/usr/local/bin/rpi-dashboard"
touch "$state/fail"
if run_activation second; then
    echo "activation_flow_test: unhealthy service was accepted" >&2
    exit 1
fi
grep -Fqx old "$prefix/bin/rpi-dashboard"
grep -Fqx private-config "$prefix/etc/xdg/rpi-dashboard/config.toml"

rm -rf "$prefix" "$test_dir/backups"
if run_activation third; then
    echo "activation_flow_test: unhealthy first install was accepted" >&2
    exit 1
fi
grep -F 'enable --now getty@tty1.service' "$state/systemctl.log" >/dev/null

printf '%s\n' unexpected >"$stage/usr/local/unexpected"
if run_activation fourth >/dev/null 2>&1; then
    echo "activation_flow_test: unexpected staged file was accepted" >&2
    exit 1
fi
