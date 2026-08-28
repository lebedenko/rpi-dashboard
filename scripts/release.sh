#!/bin/sh

set -eu

usage() {
    echo "Usage: $0 verify VERSION | archive VERSION OUTPUT_DIRECTORY" >&2
    exit 2
}

stable_version() {
    printf '%s\n' "$1" | grep -Eq '^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$'
}

project_version() {
    sed -n '/^project(RpiDashboard$/,/^)/s/^[[:space:]]*VERSION[[:space:]]*\([^[:space:]]*\).*$/\1/p' CMakeLists.txt
}

daemon_version() {
    sed -n 's/^project(DashboardDaemon VERSION \([^[:space:]]*\).*$/\1/p' daemon/CMakeLists.txt
}

verify() {
    version=$1
    stable_version "$version" || {
        echo "release: version must be stable MAJOR.MINOR.PATCH: $version" >&2
        exit 2
    }
    [ "$(project_version)" = "$version" ] || {
        echo "release: dashboard CMake version does not match $version" >&2
        exit 1
    }
    [ "$(daemon_version)" = "$version" ] || {
        echo "release: daemon CMake version does not match $version" >&2
        exit 1
    }
    grep -F "## [$version] - " CHANGELOG.md >/dev/null || {
        echo "release: CHANGELOG.md has no dated $version entry" >&2
        exit 1
    }
}

[ "$#" -ge 2 ] || usage
command=$1
version=$2
case "$command" in
    verify)
        [ "$#" -eq 2 ] || usage
        verify "$version"
        ;;
    archive)
        [ "$#" -eq 3 ] || usage
        output_dir=$3
        verify "$version"
        mkdir -p "$output_dir"
        archive="$output_dir/rpi-dashboard-$version.tar.gz"
        git archive --format=tar --prefix="rpi-dashboard-$version/" "${RELEASE_GIT_REF:-HEAD}" | gzip -n -9 >"$archive"
        (cd "$output_dir" && sha256sum "$(basename "$archive")" >"$(basename "$archive").sha256")
        ;;
    *) usage ;;
esac
