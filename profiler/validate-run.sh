#!/bin/sh

set -eu

run_dir=${1:?run directory is required}
backend=${2:?backend is required}
set -- "$run_dir"/*.qtd
trace=$1
reason=

[ "$(cat "$run_dir/exit-status.txt" 2>/dev/null || echo missing)" = 0 ] || reason="non-zero application/profiler exit"
[ -s "$trace" ] || reason="missing or empty QML trace"
if grep -Eiq 'llvmpipe|software backend|software adaptation' "$run_dir/qt.txt"; then
    reason="software rendering detected"
fi
if [ "$backend" = opengl ] && ! grep -Eiq 'V3D|Broadcom' "$run_dir/qt.txt"; then
    reason="OpenGL diagnostics did not identify V3D hardware"
fi
if [ "$backend" = vulkan ] && ! grep -Eiq 'V3DV|Broadcom' "$run_dir/qt.txt" "$run_dir/vulkan-summary.txt"; then
    reason="Vulkan diagnostics did not identify Broadcom/V3DV"
fi

if [ -n "$reason" ]; then
    printf 'rejected: %s\n' "$reason" | tee "$run_dir/validation.txt" >&2
    exit 1
fi
printf '%s\n' accepted | tee "$run_dir/validation.txt"
