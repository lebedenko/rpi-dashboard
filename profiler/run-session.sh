#!/bin/sh

set -eu

binary=${1:-}
run_dir=${RUN_DIR:?RUN_DIR is required}
backend=${QSG_RHI_BACKEND:?QSG_RHI_BACKEND is required}
trace_name=${TRACE_NAME:?TRACE_NAME is required}
qmlprofiler=/usr/lib/qt6/bin/qmlprofiler
trace="$run_dir/$trace_name"

wlr-randr --output HDMI-A-1 --transform 270
wlr-randr >"$run_dir/display.txt" 2>&1
vulkaninfo --summary >"$run_dir/vulkan-summary.txt" 2>&1 || true
if [ "$backend" = vulkan ]; then
    if ! grep -Eiq 'V3DV|Broadcom' "$run_dir/vulkan-summary.txt"; then
        echo "run-session: hardware Broadcom/V3DV Vulkan is unavailable; refusing fallback benchmark" >&2
        sed -n '/Devices:/,$p' "$run_dir/vulkan-summary.txt" >&2
        exit 3
    fi
fi

sample_system() {
    while :; do
        timestamp=$(date --iso-8601=seconds)
        for zone in /sys/class/thermal/thermal_zone*; do
            [ -r "$zone/type" ] && [ -r "$zone/temp" ] || continue
            printf '%s,%s,%s\n' "$timestamp" "$(cat "$zone/type")" "$(cat "$zone/temp")"
        done
        for freq in /sys/devices/system/cpu/cpufreq/policy*/scaling_cur_freq; do
            [ -r "$freq" ] || continue
            printf '%s,%s,%s\n' "$timestamp" "$(basename "$(dirname "$freq")")" "$(cat "$freq")"
        done
        sleep 1
    done
}

sample_system >"$run_dir/thermal-frequency.csv" &
sampler_pid=$!
trap 'kill "$sampler_pid" 2>/dev/null || true; wait "$sampler_pid" 2>/dev/null || true' EXIT HUP INT TERM

export QT_QPA_PLATFORM=wayland
export QT_FORCE_STDERR_LOGGING=1
export QSG_INFO=1
export QSG_RHI_BACKEND="$backend"

perf stat -o "$run_dir/perf.txt" \
    -e task-clock,cycles,instructions,context-switches,cpu-migrations,page-faults \
    "$qmlprofiler" --include scenegraph,animations,painting,pixmapcache,inputevents \
    -o "$trace" -- "$binary" 2>"$run_dir/qt.txt" &
profile_pid=$!
# Capture all processes because perf starts qmlprofiler, which then starts the
# dashboard. The aggregate report selects the dashboard rows and derives its
# maximum RSS and CPU samples.
pidstat -h -r -u -p ALL 1 >"$run_dir/pidstat.txt" &
pidstat_pid=$!
set +e
wait "$profile_pid"
status=$?
set -e
kill "$pidstat_pid" 2>/dev/null || true
wait "$pidstat_pid" 2>/dev/null || true
printf '%s\n' "$status" >"$run_dir/exit-status.txt"
exit "$status"
