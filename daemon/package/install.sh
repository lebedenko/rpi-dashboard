#!/bin/sh
set -eu
[ "$(id -u)" -eq 0 ] || { echo "install.sh must run as root" >&2; exit 1; }
case "$(uname -m)" in x86_64|aarch64) ;; *) echo "unsupported architecture" >&2; exit 1;; esac
command -v systemctl >/dev/null 2>&1 || { echo "systemd is required" >&2; exit 1; }
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
install -d -m 0755 /usr/local/bin /usr/local/lib/systemd/system /etc/xdg/dashboard-daemon
install -m 0755 "$src/dashboard-daemon" /usr/local/bin/.dashboard-daemon.new
mv /usr/local/bin/.dashboard-daemon.new /usr/local/bin/dashboard-daemon
install -m 0644 "$src/dashboard-daemon.service" /usr/local/lib/systemd/system/.dashboard-daemon.service.new
mv /usr/local/lib/systemd/system/.dashboard-daemon.service.new /usr/local/lib/systemd/system/dashboard-daemon.service
if [ ! -e /etc/xdg/dashboard-daemon/config.toml ]; then install -m 0644 "$src/config.toml" /etc/xdg/dashboard-daemon/.config.toml.new; mv /etc/xdg/dashboard-daemon/.config.toml.new /etc/xdg/dashboard-daemon/config.toml; fi
systemctl daemon-reload
echo "Installed but not enabled or started. Configure, validate, then run: systemctl enable --now dashboard-daemon"
