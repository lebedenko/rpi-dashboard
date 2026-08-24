#!/bin/sh

set -eu

dashboard_binary=${1:-}

if [ -z "$dashboard_binary" ] || [ ! -x "$dashboard_binary" ]; then
  echo "dashboard-session: dashboard executable not found or not executable: $dashboard_binary" >&2
  exit 126
fi

if ! command -v wlr-randr >/dev/null 2>&1; then
  echo "dashboard-session: wlr-randr is not installed or not available in PATH" >&2
  exit 127
fi

if ! wlr-randr --output HDMI-A-1 --transform 270; then
  echo "dashboard-session: failed to configure HDMI-A-1 with transform 270" >&2
  exit 1
fi

exec "$dashboard_binary"
