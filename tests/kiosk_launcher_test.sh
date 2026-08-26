#!/bin/sh

set -eu

launcher=$1
session_helper=$2
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/rpi-dashboard-kiosk-test-XXXXXX")
trap 'rm -rf -- "$test_dir"' EXIT HUP INT TERM

fake_bin="$test_dir/bin"
mkdir "$fake_bin"

cat >"$fake_bin/cage" <<'EOF'
#!/bin/sh
printf '%s\n' "$@" >"$TEST_STATE/cage.args"
while [ "$#" -gt 0 ] && [ "$1" != "--" ]; do
    shift
done
[ "$#" -gt 0 ] && shift
exec "$@"
EOF

cat >"$fake_bin/wlr-randr" <<'EOF'
#!/bin/sh
printf '%s\n' "$@" >"$TEST_STATE/wlr-randr.args"
exit "${WLR_RANDR_STATUS:-0}"
EOF

cat >"$test_dir/dashboard" <<'EOF'
#!/bin/sh
exit 37
EOF

chmod +x "$fake_bin/cage" "$fake_bin/wlr-randr" "$test_dir/dashboard"

set +e
TEST_STATE=$test_dir PATH="$fake_bin:/usr/bin:/bin" "$launcher" "$test_dir/dashboard" \
  >"$test_dir/stdout" 2>"$test_dir/stderr"
status=$?
set -e

[ "$status" -eq 37 ] || {
  echo "kiosk_launcher_test: expected dashboard exit 37, got $status" >&2
  exit 1
}

cat >"$test_dir/expected-cage.args" <<EOF
-s
-d
--
$session_helper
$test_dir/dashboard
EOF
cmp "$test_dir/expected-cage.args" "$test_dir/cage.args"

cat >"$test_dir/expected-wlr-randr.args" <<'EOF'
--output
HDMI-A-1
--transform
270
EOF
cmp "$test_dir/expected-wlr-randr.args" "$test_dir/wlr-randr.args"

set +e
TEST_STATE=$test_dir WLR_RANDR_STATUS=9 PATH="$fake_bin:/usr/bin:/bin" \
  "$session_helper" "$test_dir/dashboard" >"$test_dir/stdout" 2>"$test_dir/stderr"
status=$?
set -e

[ "$status" -eq 1 ] || {
  echo "kiosk_launcher_test: expected output configuration exit 1, got $status" >&2
  exit 1
}
grep -Fqx 'dashboard-session: failed to configure HDMI-A-1 with transform 270' \
  "$test_dir/stderr"

empty_path="$test_dir/empty-path"
mkdir "$empty_path"
set +e
PATH="$empty_path" "$session_helper" "$test_dir/dashboard" \
  >"$test_dir/stdout" 2>"$test_dir/stderr"
status=$?
set -e

[ "$status" -eq 127 ] || {
  echo "kiosk_launcher_test: expected missing wlr-randr exit 127, got $status" >&2
  exit 1
}
grep -Fqx 'dashboard-session: wlr-randr is not installed or not available in PATH' \
  "$test_dir/stderr"
