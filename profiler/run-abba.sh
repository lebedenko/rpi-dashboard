#!/bin/sh

set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir="$repo_dir/build/profile"
binary="$build_dir/src/dashboard/holonight-dashboard"
qmlprofiler=/usr/lib/qt6/bin/qmlprofiler
sequence="opengl vulkan vulkan opengl"

if [ ! -t 0 ]; then
    echo "run-abba: run this from an active physical TTY, not SSH or a pipe" >&2
    exit 2
fi
if [ -n "${SSH_CONNECTION:-}" ] || [ -n "${SSH_TTY:-}" ]; then
    echo "run-abba: SSH sessions are not valid for the Cage benchmark" >&2
    exit 2
fi
for tool in cmake ninja cage wlr-randr perf pidstat vulkaninfo; do
    command -v "$tool" >/dev/null 2>&1 || { echo "run-abba: missing tool: $tool" >&2; exit 127; }
done
[ -x "$qmlprofiler" ] || { echo "run-abba: missing qmlprofiler: $qmlprofiler" >&2; exit 127; }

cmake -S "$repo_dir" -B "$build_dir" -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=OFF \
    -DCMAKE_CXX_FLAGS=-DQT_QML_DEBUG -DCMAKE_PREFIX_PATH=/usr/lib/qt6
cmake --build "$build_dir"
[ -x "$binary" ] || { echo "run-abba: profile binary was not produced" >&2; exit 126; }

study_id=$(date +%Y%m%d-%H%M%S)
study_dir="$repo_dir/profiler/runs/$study_id"
mkdir -p "$study_dir"
printf '%s\n' "$sequence" >"$study_dir/sequence.txt"

run_number=0
for backend in $sequence; do
    run_number=$((run_number + 1))
    run_dir="$study_dir/run-$run_number-$backend"
    mkdir -p "$run_dir"
    if [ "$run_number" -gt 1 ]; then
        echo "Wait until CPU temperature is within 2 C of run 1's starting value."
    fi
    echo "Run $run_number/4: $backend. Press Enter when thermally ready."
    read -r _ready
    echo "Use a stopwatch and follow profiler/workload.md exactly; close normally at 90 s."
    printf '%s\n' "$backend" >"$run_dir/backend.txt"
    trace_name="qmlprofiler-trace-holonight-dashboard-$study_id-run-$run_number-$backend.qtd"
    if ! RUN_DIR="$run_dir" TRACE_NAME="$trace_name" QSG_RHI_BACKEND="$backend" QSG_INFO=1 \
        cage -s -d -- "$repo_dir/profiler/run-session.sh" "$binary"; then
        echo "run-abba: run $run_number exited abnormally; study stopped" >&2
        exit 1
    fi
    "$repo_dir/profiler/validate-run.sh" "$run_dir" "$backend"
done

echo "Capture complete: $study_dir"
echo "Generate standalone reports for each .qtd trace before aggregating results."
