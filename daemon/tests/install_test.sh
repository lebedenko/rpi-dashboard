#!/bin/sh
set -eu
installer=$1
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/dashboard-daemon-install-test-XXXXXX")
trap 'rm -rf -- "$test_dir"' EXIT HUP INT TERM
package="$test_dir/package"; root="$test_dir/root"; state="$test_dir/state"; bin="$test_dir/bin"
mkdir "$package" "$state" "$bin"
cp "$installer" "$package/install.sh"
cp "$(dirname "$installer")/config.toml" "$(dirname "$installer")/dashboard-daemon.service" "$package/"
cat >"$package/dashboard-daemon" <<'EOF'
#!/bin/sh
case " $* " in *' --check-config '*) grep -q 'host = ""' "$2" && exit 1 || exit 0;; *' --version '*) echo 'dashboard-daemon 0.1.1';; esac
EOF
cat >"$bin/systemctl" <<'EOF'
#!/bin/sh
printf '%s\n' "$*" >>"$TEST_STATE/systemctl.log"
case "$1" in
  is-enabled) [ -f "$TEST_STATE/enabled" ];;
  is-active) [ -f "$TEST_STATE/active" ] && echo active;;
  show) case "$*" in *MainPID*) echo 4242;; *) echo 0;; esac;;
  enable) touch "$TEST_STATE/enabled"; case " $* " in *' --now '*) touch "$TEST_STATE/active";; esac;;
  restart|start) touch "$TEST_STATE/active";;
  stop) rm -f "$TEST_STATE/active";;
  disable) rm -f "$TEST_STATE/enabled";;
esac
EOF
chmod +x "$package/dashboard-daemon" "$package/install.sh" "$bin/systemctl"
run_install() { TEST_STATE=$state DASHBOARD_DAEMON_ALLOW_UNPRIVILEGED=1 DASHBOARD_DAEMON_PACKAGE_ARCH=x86_64 DASHBOARD_DAEMON_HOST_ARCH=x86_64 DASHBOARD_DAEMON_INSTALL_ROOT=$root DASHBOARD_DAEMON_SYSTEMCTL=$bin/systemctl DASHBOARD_DAEMON_HEALTH_INTERVAL=0 "$package/install.sh"; }
sed 's/host = ""/host = "127.0.0.1"/' "$package/config.toml" >"$package/config.toml.valid"; mv "$package/config.toml.valid" "$package/config.toml"
run_install
[ -x "$root/usr/local/bin/dashboard-daemon" ]; [ -f "$state/enabled" ]; [ -f "$state/active" ]
printf '%s\n' '[dashboard]' 'host = "192.0.2.10"' 'port = 51337' '[telemetry]' 'interval_seconds = 2' 'display_name = "preserved"' >"$root/etc/xdg/dashboard-daemon/config.toml"
run_install
grep -Fqx 'host = "192.0.2.10"' "$root/etc/xdg/dashboard-daemon/config.toml"
grep -Fqx 'restart dashboard-daemon.service' "$state/systemctl.log"
if TEST_STATE=$state DASHBOARD_DAEMON_ALLOW_UNPRIVILEGED=1 DASHBOARD_DAEMON_PACKAGE_ARCH=aarch64 DASHBOARD_DAEMON_HOST_ARCH=x86_64 DASHBOARD_DAEMON_INSTALL_ROOT=$root DASHBOARD_DAEMON_SYSTEMCTL=$bin/systemctl "$package/install.sh" >/dev/null 2>&1; then
  echo "install_test: incompatible architecture accepted" >&2; exit 1
fi
