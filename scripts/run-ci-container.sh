#!/bin/sh

set -eu

usage() {
    echo "Usage: $0 {dev|tidy|asan|ubsan}" >&2
    exit 2
}

[ "$#" -eq 1 ] || usage
preset=$1
case "$preset" in
    dev|tidy|asan|ubsan) ;;
    *) usage ;;
esac

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
lock_file=${CI_IMAGE_LOCK_FILE:-$source_dir/ci/image.env}
docker_command=${DOCKER_COMMAND:-docker}

if ! command -v "$docker_command" >/dev/null 2>&1; then
    echo "run-ci-container: Docker command '$docker_command' was not found" >&2
    exit 127
fi

image=${CI_IMAGE_OVERRIDE:-}
if [ -z "$image" ]; then
    if [ ! -r "$lock_file" ]; then
        echo "run-ci-container: image lock is not readable: $lock_file" >&2
        exit 2
    fi
    image=$(sed -n 's/^CI_IMAGE=//p' "$lock_file")
    case "$image" in
        ghcr.io/lebedenko/rpi-dashboard-ci@sha256:*) ;;
        UNPUBLISHED)
            echo "run-ci-container: CI image is not published; see docs/specs/016-reproducible-ci-toolchain.md" >&2
            exit 2
            ;;
        *)
            echo "run-ci-container: invalid immutable image reference in $lock_file" >&2
            exit 2
            ;;
    esac
    digest=${image##*@sha256:}
    if [ "${#digest}" -ne 64 ] || printf '%s\n' "$digest" | grep -Eqv '^[0-9a-f]{64}$'; then
        echo "run-ci-container: invalid SHA-256 digest in $lock_file" >&2
        exit 2
    fi
fi

build_dir=$(mktemp -d "${TMPDIR:-/tmp}/rpi-dashboard-ci-${preset}-XXXXXX")
trap 'rm -rf -- "$build_dir"' EXIT HUP INT TERM

echo "CI image: $image"
"$docker_command" run --rm \
    --user "$(id -u):$(id -g)" \
    --env HOME=/tmp \
    --mount "type=bind,src=$source_dir,dst=/workspace,readonly" \
    --mount "type=bind,src=$build_dir,dst=/workspace/build" \
    --workdir /workspace \
    "$image" \
    sh -c 'verify-ci-toolchain && cmake --preset "$1" && cmake --build --preset "$1" && ctest --preset "$1"' \
    sh "$preset"
