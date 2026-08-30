#!/bin/sh

set -eu

usage() { echo "Usage: $0 generate VERSION OUTPUT" >&2; exit 2; }
stable_version() { printf '%s\n' "$1" | grep -Eq '^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$'; }

latest_release_tag() {
    gh release list --exclude-drafts --exclude-pre-releases --limit 100 \
        --json tagName --jq '.[].tagName' |
        while IFS= read -r tag; do
            version=${tag#v}
            if [ "v$version" = "$tag" ] && stable_version "$version"; then
                printf '%s\n' "$tag"
                break
            fi
        done
}

generate() {
    version=$1 output=$2
    stable_version "$version" || { echo "changelog: version must be stable MAJOR.MINOR.PATCH: $version" >&2; exit 2; }
    baseline=$(latest_release_tag)
    [ -n "$baseline" ] || { echo "changelog: no published stable GitHub Release is available" >&2; exit 1; }
    git rev-parse --verify --quiet "$baseline^{commit}" >/dev/null || {
        echo "changelog: published baseline $baseline is unavailable locally" >&2
        exit 1
    }
    baseline_version=${baseline#v}
    grep -Fqx "## [$baseline_version]" CHANGELOG.md ||
        grep -Fq "## [$baseline_version] - " CHANGELOG.md || {
            echo "changelog: CHANGELOG.md has no published $baseline_version section" >&2
            exit 1
        }

    release_date=${RELEASE_DATE:-$(date -u +%Y-%m-%d)}
    printf '%s\n' "$release_date" | grep -Eq '^[0-9]{4}-[0-9]{2}-[0-9]{2}$' || {
        echo "changelog: RELEASE_DATE must be a UTC YYYY-MM-DD date" >&2
        exit 2
    }
    runner=${GIT_CLIFF_RUNNER:-scripts/run-git-cliff.sh}
    raw_body=$(mktemp "${TMPDIR:-/tmp}/rpi-dashboard-changelog-raw-XXXXXX")
    body=$(mktemp "${TMPDIR:-/tmp}/rpi-dashboard-changelog-body-XXXXXX")
    trap 'rm -f -- "$raw_body" "$body"' EXIT HUP INT TERM
    "$runner" --config cliff.toml --ignore-tags '^v.*$' --tag "v$version" "$baseline..HEAD" >"$raw_body"
    awk 'NF { if (blank && wrote) print ""; print; blank=0; wrote=1; next } { blank=1 }' "$raw_body" >"$body"
    [ -s "$body" ] || { echo "changelog: no publishable Conventional Commits after $baseline" >&2; exit 1; }

    {
        sed -n '1,/^## \[/p' CHANGELOG.md | sed '$d'
        printf '## [%s] - %s\n\n' "$version" "$release_date"
        cat "$body"
        printf '\n'
        sed -n "/^## \[$baseline_version\]/,/^\[/p" CHANGELOG.md | sed '$d'
        printf '[%s]: https://github.com/lebedenko/rpi-dashboard/compare/%s...v%s\n' "$version" "$baseline" "$version"
        sed -n "/^\[$baseline_version\]:/,\$p" CHANGELOG.md
    } >"$output"
}

[ "$#" -eq 3 ] || usage
[ "$1" = generate ] || usage
generate "$2" "$3"
