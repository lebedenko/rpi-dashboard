#!/bin/sh

set -eu

launcher=$1
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/rpi-dashboard-clang-tidy-test-XXXXXX")
trap 'rm -rf -- "$test_dir"' EXIT HUP INT TERM

fake_tidy="$test_dir/clang-tidy"
actual_arguments="$test_dir/actual.arguments"
expected_arguments="$test_dir/expected.arguments"

cat >"$fake_tidy" <<'EOF'
#!/bin/sh
printf '%s\0' "$@" >"$TEST_ARGUMENTS"
exit "${TEST_EXIT_STATUS:-0}"
EOF
chmod +x "$fake_tidy"

printf '%s\0' \
  '--checks=clang-diagnostic-*' \
  '/tmp/source file.cpp' \
  '--' \
  '-std=c++23' \
  '-DVALUE=with spaces' \
  '-mno-direct-extern-access=retained' \
  '' >"$expected_arguments"

set +e
TEST_ARGUMENTS="$actual_arguments" TEST_EXIT_STATUS=23 \
  "$launcher" "$fake_tidy" \
    '--checks=clang-diagnostic-*' \
    '-mno-direct-extern-access' \
    '/tmp/source file.cpp' \
    '--' \
    '-std=c++23' \
    '-DVALUE=with spaces' \
    '-mno-direct-extern-access=retained' \
    '-mno-direct-extern-access' \
    ''
status=$?
set -e

[ "$status" -eq 23 ] || {
  echo "clang_tidy_launcher_test: expected clang-tidy exit 23, got $status" >&2
  exit 1
}

cmp "$expected_arguments" "$actual_arguments"
