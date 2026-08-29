#!/bin/sh

set -eu

[ "$(id -u)" -ne 0 ] || {
    echo "install.sh: run as an unprivileged user; sudo is invoked only for host installation" >&2
    exit 1
}
source_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$source_dir"
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
cmake --preset release
cmake --build --preset release
sudo cmake --install build/release
sudo /usr/local/libexec/rpi-dashboard/provision.sh
