#!/bin/sh

set -eu

cmake_command=$1
build_directory=$2
source_directory=$3
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/rpi-dashboard-install-test-XXXXXX")
trap 'rm -rf -- "$test_dir"' EXIT HUP INT TERM

install_root="$test_dir/root"
installed_config="$install_root/usr/local/etc/xdg/rpi-dashboard/config.toml"
mkdir -p "$(dirname "$installed_config")"
cat >"$installed_config" <<'EOF'
[existing]
value = "preserve me"
EOF

DESTDIR="$install_root" "$cmake_command" --install "$build_directory" --prefix /usr/local
DESTDIR="$install_root" "$cmake_command" --install "$build_directory" --prefix /usr/local

installed_bin="$install_root/usr/local/bin/rpi-dashboard"
installed_libexec="$install_root/usr/local/libexec/rpi-dashboard"
installed_service="$install_root/usr/local/lib/systemd/system/rpi-dashboard.service"

[ -x "$installed_bin" ]
[ -x "$installed_libexec/run-kiosk.sh" ]
[ -x "$installed_libexec/run-dashboard-session.sh" ]
[ -x "$installed_libexec/provision.sh" ]
[ -f "$installed_service" ]
grep -Fqx '[existing]' "$installed_config"
grep -Fqx 'value = "preserve me"' "$installed_config"
[ "$(grep -Fxc '[weather]' "$installed_config")" -eq 1 ]
[ "$(grep -Fxc '[weather.location]' "$installed_config")" -eq 1 ]
cmp "$source_directory/install/rpi-dashboard.service" "$installed_service"
grep -Fqx 'OnSuccess=getty@tty1.service' "$installed_service"
sh -n "$installed_libexec/run-kiosk.sh"
sh -n "$installed_libexec/run-dashboard-session.sh"
sh -n "$installed_libexec/provision.sh"

fake_bin="$test_dir/bin"
state="$test_dir/state"
mkdir "$fake_bin" "$state"
printf '%s\n' 'Raspberry Pi 5 Model B Rev 1.0' >"$test_dir/model"

cat >"$fake_bin/uname" <<'EOF'
#!/bin/sh
printf '%s\n' aarch64
EOF
cat >"$fake_bin/cage" <<'EOF'
#!/bin/sh
exit 0
EOF
cat >"$fake_bin/wlr-randr" <<'EOF'
#!/bin/sh
exit 0
EOF
cat >"$fake_bin/getent" <<'EOF'
#!/bin/sh
case "$1:$2" in
  passwd:dashboard)
    [ -f "$TEST_STATE/account" ] || exit 2
    printf '%s\n' 'dashboard:x:1001:1001::/home/dashboard:/usr/sbin/nologin'
    ;;
  group:1001) printf '%s\n' 'dashboard:x:1001:' ;;
  *) exit 2 ;;
esac
EOF
cat >"$fake_bin/id" <<'EOF'
#!/bin/sh
[ "$1" = -Gn ] && [ "$2" = dashboard ] && printf '%s\n' dashboard
EOF
cat >"$fake_bin/useradd" <<'EOF'
#!/bin/sh
printf '%s\n' "$*" >>"$TEST_STATE/useradd.log"
touch "$TEST_STATE/account"
EOF
cat >"$fake_bin/passwd" <<'EOF'
#!/bin/sh
case "$1" in
  --lock) printf '%s\n' "$*" >>"$TEST_STATE/passwd.log" ;;
  -S) printf '%s\n' 'dashboard L 2026-08-26 0 99999 7 -1' ;;
  *) exit 2 ;;
esac
EOF
cat >"$fake_bin/systemctl" <<'EOF'
#!/bin/sh
printf '%s\n' "$*" >>"$TEST_STATE/systemctl.log"
EOF
chmod +x "$fake_bin"/*

run_provision() {
  TEST_STATE="$state" \
    RPI_DASHBOARD_ALLOW_UNPRIVILEGED=1 \
    RPI_DASHBOARD_SYSTEMD_AVAILABLE=1 \
    RPI_DASHBOARD_MODEL_FILE="$test_dir/model" \
    PATH="$fake_bin:/usr/bin:/bin" \
    "$installed_libexec/provision.sh"
}

run_provision
run_provision

[ "$(wc -l <"$state/useradd.log")" -eq 1 ]
[ "$(wc -l <"$state/passwd.log")" -eq 1 ]
[ "$(grep -Fxc 'daemon-reload' "$state/systemctl.log")" -eq 2 ]
[ "$(grep -Fxc 'enable rpi-dashboard.service' "$state/systemctl.log")" -eq 2 ]
if grep -Eq '(^| )(start|restart)( |$)' "$state/systemctl.log"; then
  echo "install_flow_test: provisioning started the service" >&2
  exit 1
fi
