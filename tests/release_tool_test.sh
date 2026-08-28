#!/bin/sh

set -eu

release_script=$1
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/rpi-dashboard-release-test-XXXXXX")
trap 'rm -rf -- "$test_dir"' EXIT HUP INT TERM
repo=$test_dir/repo
mkdir "$repo"
cp "$release_script" "$repo/release.sh"
cat >"$repo/CMakeLists.txt" <<'EOF'
project(RpiDashboard
    VERSION 0.1.0
)
EOF
mkdir "$repo/daemon"
printf '%s\n' 'project(DashboardDaemon VERSION 0.1.0 LANGUAGES CXX)' >"$repo/daemon/CMakeLists.txt"
printf '%s\n' '# Changelog' '' '## [0.1.0] - 2026-08-28' '' '- Initial release.' >"$repo/CHANGELOG.md"
(cd "$repo" && git init -q && git add . && git -c user.name=test -c user.email=test@example.invalid commit -qm test)

(cd "$repo" && sh release.sh verify 0.1.0)
if (cd "$repo" && sh release.sh verify 0.1 >/dev/null 2>&1); then
    echo "release_tool_test: accepted an unstable version" >&2
    exit 1
fi
(cd "$repo" && sh release.sh archive 0.1.0 "$test_dir/one")
(cd "$repo" && sh release.sh archive 0.1.0 "$test_dir/two")
cmp "$test_dir/one/rpi-dashboard-0.1.0.tar.gz" "$test_dir/two/rpi-dashboard-0.1.0.tar.gz"
(cd "$test_dir/one" && sha256sum --check rpi-dashboard-0.1.0.tar.gz.sha256)
tar -tzf "$test_dir/one/rpi-dashboard-0.1.0.tar.gz" | grep -Fqx 'rpi-dashboard-0.1.0/CHANGELOG.md'
