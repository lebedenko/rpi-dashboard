#!/bin/sh

set -eu

usage() { echo "Usage: $0 {prepare|check} VERSION" >&2; exit 2; }
stable_version() { printf '%s\n' "$1" | grep -Eq '^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$'; }
version_gt() { awk -v a="$1" -v b="$2" 'BEGIN { split(a,x,"."); split(b,y,"."); for(i=1;i<=3;i++){ if((x[i]+0)>(y[i]+0))exit 0; if((x[i]+0)<(y[i]+0))exit 1 } exit 1 }'; }
root_version() { sed -n '1p' VERSION; }
ensure_unused() {
    version=$1
    ! git rev-parse --verify --quiet "refs/tags/v$version" >/dev/null || { echo "release: tag v$version already exists" >&2; exit 1; }
    if git remote get-url origin >/dev/null 2>&1 &&
        [ -n "$(git ls-remote --tags origin "refs/tags/v$version" | sed -n '1p')" ]; then
        echo "release: remote tag v$version already exists" >&2
        exit 1
    fi
    if gh release view "v$version" >/dev/null 2>&1; then
        echo "release: GitHub Release v$version already exists" >&2
        exit 1
    fi
}
validate_metadata() {
    version=$1
    [ "$(root_version)" = "$version" ] || { echo "release: VERSION does not match $version" >&2; exit 1; }
    grep -Eq "^## \\[$version\\] - [0-9]{4}-[0-9]{2}-[0-9]{2}$" CHANGELOG.md || {
        echo "release: CHANGELOG.md has no UTC-dated $version section" >&2
        exit 1
    }
}
prepare() {
    version=$1 current=$(root_version)
    branch=$(git branch --show-current)
    [ -n "$branch" ] && [ "$branch" != main ] || { echo "release: prepare requires a non-main branch" >&2; exit 1; }
    [ -z "$(git status --porcelain)" ] || { echo "release: prepare requires a clean worktree" >&2; exit 1; }
    version_gt "$version" "$current" || { echo "release: version $version must be greater than $current" >&2; exit 1; }
    ensure_unused "$version"
    output=$(mktemp "${TMPDIR:-/tmp}/rpi-dashboard-changelog-XXXXXX")
    trap 'rm -f -- "$output"' EXIT HUP INT TERM
    scripts/changelog.sh generate "$version" "$output"
    printf '%s\n' "$version" >VERSION
    mv "$output" CHANGELOG.md
    trap - EXIT HUP INT TERM
}
check() {
    version=$1
    validate_metadata "$version"
    release_date=$(sed -n "s/^## \\[$version\\] - \\([0-9-]*\\)$/\\1/p" CHANGELOG.md)
    output=$(mktemp "${TMPDIR:-/tmp}/rpi-dashboard-changelog-check-XXXXXX")
    trap 'rm -f -- "$output"' EXIT HUP INT TERM
    RELEASE_DATE=$release_date scripts/changelog.sh generate "$version" "$output"
    cmp CHANGELOG.md "$output" || { echo "release: CHANGELOG.md differs from deterministic generation" >&2; exit 1; }
    sh scripts/release.sh verify "$version"
}

[ "$#" -eq 2 ] || usage
stable_version "$2" || { echo "release: version must be stable MAJOR.MINOR.PATCH: $2" >&2; exit 2; }
case "$1" in prepare) prepare "$2" ;; check) check "$2" ;; *) usage ;; esac
