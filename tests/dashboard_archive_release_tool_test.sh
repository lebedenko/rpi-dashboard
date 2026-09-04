#!/bin/sh

set -eu

release_script=$1
source_root=$(CDPATH= cd -- "$(dirname -- "$release_script")/.." && pwd)
release_tool_test=$source_root/tests/release_tool_test.sh
version=$(sed -n '1p' "$source_root/VERSION")
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/rpi-dashboard-archive-test-XXXXXX")
trap 'rm -rf -- "$test_dir"' EXIT HUP INT TERM

output=$test_dir/output
(cd "$source_root" && RELEASE_GIT_REF=HEAD sh "$release_script" dashboard "$version" "$output")
tar --extract --gzip --file "$output/rpi-dashboard-$version.tar.gz" --directory "$test_dir"
source_dir=$test_dir/rpi-dashboard-$version

(cd "$source_dir" && sh "$release_tool_test" scripts/release.sh)
