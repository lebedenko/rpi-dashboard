#!/bin/sh

set -eu

expect_line() {
    description=$1
    expected=$2
    actual=$3
    if [ "$actual" != "$expected" ]; then
        echo "verify-ci-toolchain: expected $description '$expected', got '$actual'" >&2
        exit 1
    fi
    printf '%-12s %s\n' "$description" "$actual"
}

. /etc/os-release
expect_line Ubuntu 24.04 "$VERSION_ID"
expect_line GCC 13.3.0 "$(gcc-13 -dumpfullversion)"
expect_line G++ 13.3.0 "$(g++-13 -dumpfullversion)"
expect_line clang-tidy 18.1.3 "$(clang-tidy-18 --version | sed -n 's/.*version \([0-9][^ ]*\).*/\1/p')"
expect_line CMake 3.31.6 "$(cmake --version | sed -n '1s/cmake version //p')"
expect_line Ninja 1.13.2 "$(ninja --version)"
expect_line Qt 6.8.3 "$(qmake -query QT_VERSION)"
