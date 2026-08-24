#!/bin/sh

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
dashboard_binary=${1:-"$script_dir/../build/release/src/dashboard/holonight-dashboard"}

if ! command -v cage >/dev/null 2>&1; then
    echo "run-kiosk: Cage is not installed or not available in PATH" >&2
    exit 127
fi

if [ ! -x "$dashboard_binary" ]; then
    echo "run-kiosk: dashboard executable not found or not executable: $dashboard_binary" >&2
    exit 126
fi

if [ -z "${XDG_RUNTIME_DIR:-}" ]; then
    XDG_RUNTIME_DIR=$(mktemp -d "/tmp/holonight-runtime-$(id -u)-XXXXXX")
    chmod 700 "$XDG_RUNTIME_DIR"
    export XDG_RUNTIME_DIR
fi

export QT_QPA_PLATFORM=wayland
export QT_FORCE_STDERR_LOGGING=1

exec cage -s -d -- "$dashboard_binary"
