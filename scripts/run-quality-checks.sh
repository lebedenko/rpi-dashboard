#!/bin/sh

set -eu

preset=${1:-dev}
case "$preset" in
  dev|tidy) ;;
  *) echo "Usage: $0 [dev|tidy]" >&2; exit 2 ;;
esac

temporary_directory=$(mktemp -d "${TMPDIR:-/tmp}/rpi-dashboard-quality-XXXXXX")
cleanup() { rm -rf -- "$temporary_directory"; }
trap cleanup EXIT HUP INT TERM

qt_version=$(
  if command -v qmake6 >/dev/null 2>&1; then
    qmake6 -query QT_VERSION
  elif command -v qmake >/dev/null 2>&1; then
    qmake -query QT_VERSION
  fi
)
if [ "$qt_version" = 6.8.3 ]; then
  qmlformat=$(command -v qmlformat)
  find src tests -type f -name '*.qml' -exec sh -c '
    temporary_directory=$1
    qmlformat=$2
    shift 2
    for source_file do
      formatted_file="$temporary_directory/formatted.qml"
      if ! "$qmlformat" "$source_file" >"$formatted_file" ||
         ! diff -u "$source_file" "$formatted_file"; then
        touch "$temporary_directory/qmlformat-failed"
      fi
    done
  ' sh "$temporary_directory" "$qmlformat" {} +
  test ! -e "$temporary_directory/qmlformat-failed"
else
  echo "Skipping QML format check with Qt $qt_version; CI enforces the pinned Qt 6.8.3 formatter."
fi

cmake --preset "$preset"
cmake --build --preset "$preset" --target rpi-dashboard-qml
cmake --build --preset "$preset" --target all_qmllint
cmake -S daemon -B build/daemon-quality -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build/daemon-quality
ctest --test-dir build/daemon-quality --output-on-failure
