#!/bin/sh

set -eu

[ "$#" -eq 2 ] || {
    echo "Usage: $0 ARCHIVE CHECKSUM" >&2
    exit 2
}
archive=$1
checksum=$2
work=$(mktemp -d "${TMPDIR:-/tmp}/rpi-dashboard-deploy-XXXXXX")
trap 'rm -rf -- "$work"' EXIT HUP INT TERM

(cd "$(dirname "$archive")" && sha256sum --check "$(basename "$checksum")")
tar --extract --gzip --file "$archive" --directory "$work"
source_dir=$(find "$work" -mindepth 1 -maxdepth 1 -type d)
[ -n "$source_dir" ] && [ "$(printf '%s\n' "$source_dir" | wc -l)" -eq 1 ]

(cd "$source_dir" && cmake --preset dev && cmake --build --preset dev && ctest --preset dev)
(cd "$source_dir" && cmake --preset release && cmake --build --preset release)
DESTDIR="$work/stage" cmake --install "$source_dir/build/release" --prefix /usr/local
sudo /usr/local/libexec/rpi-dashboard/activate-release.sh "$work/stage"
