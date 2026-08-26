#!/bin/sh

set -eu

launcher=$1
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/rpi-dashboard-clang-tidy-test-XXXXXX")
trap 'rm -rf -- "$test_dir"' EXIT HUP INT TERM

fake_tidy="$test_dir/clang-tidy"
actual_arguments="$test_dir/actual.arguments"
expected_arguments="$test_dir/expected.arguments"
tidy_called="$test_dir/tidy.called"
build_directory="$test_dir/build directory"
mkdir "$build_directory"

cat >"$fake_tidy" <<'EOF'
#!/bin/sh
touch "$TEST_TIDY_CALLED"
printf '%s\0' "$@" >"$TEST_ARGUMENTS"
exit "${TEST_EXIT_STATUS:-0}"
EOF
chmod +x "$fake_tidy"

printf '%s\0' \
  '--checks=clang-diagnostic-*' \
  '/tmp/source file.cpp' \
  '--' \
  '-std=c++23' \
  "$build_directory/generated header.h" \
  '-DVALUE=with spaces' \
  '-mno-direct-extern-access=retained' \
  '' >"$expected_arguments"

set +e
TEST_ARGUMENTS="$actual_arguments" TEST_TIDY_CALLED="$tidy_called" TEST_EXIT_STATUS=23 \
  "$launcher" "$fake_tidy" "$build_directory" \
    '--checks=clang-diagnostic-*' \
    '-mno-direct-extern-access' \
    '/tmp/source file.cpp' \
    '--' \
    '-std=c++23' \
    "$build_directory/generated header.h" \
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

rm "$tidy_called"
TEST_ARGUMENTS="$actual_arguments" TEST_TIDY_CALLED="$tidy_called" \
  "$launcher" "$fake_tidy" "$build_directory" \
    '--checks=clang-diagnostic-*' \
    "$build_directory/src/dashboard/.qt/rcc/qrc_resources_init.cpp" \
    '--' \
    '-std=c++23'

[ ! -e "$tidy_called" ] || {
  echo "clang_tidy_launcher_test: clang-tidy ran for a generated translation unit" >&2
  exit 1
}
