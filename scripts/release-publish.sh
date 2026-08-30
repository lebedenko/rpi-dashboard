#!/bin/sh
set -eu
usage() { echo "Usage: $0 VERSION" >&2; exit 2; }
stable_version() { printf '%s\n' "$1" | grep -Eq '^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$'; }
[ "$#" -eq 1 ] || usage
version=$1
stable_version "$version" || { echo "release: version must be stable MAJOR.MINOR.PATCH: $version" >&2; exit 2; }
[ "$(git branch --show-current)" = main ] || { echo "release: publish requires main" >&2; exit 1; }
[ -z "$(git status --porcelain)" ] || { echo "release: publish requires a clean worktree" >&2; exit 1; }
git fetch --no-tags origin main
sha=$(git rev-parse HEAD)
[ "$sha" = "$(git rev-parse origin/main)" ] || { echo "release: main must be synchronized with origin/main" >&2; exit 1; }
scripts/release-prepare.sh check "$version"
ci_run=$(gh run list --workflow ci.yml --branch main --commit "$sha" --status success --limit 1 --json databaseId,headSha --jq ".[] | select(.headSha == \"$sha\") | .databaseId" | sed -n '1p')
[ -n "$ci_run" ] || { echo "release: no successful CI run exists for $sha" >&2; exit 1; }
tag="v$version"
[ -z "$(git ls-remote --tags origin "refs/tags/$tag" | sed -n '1p')" ] || { echo "release: remote tag $tag already exists" >&2; exit 1; }
release_state=$(gh api "repos/{owner}/{repo}/releases" --paginate --jq ".[] | select(.tag_name == \"$tag\") | [.draft, .target_commitish] | @tsv" | sed -n '1p')
if [ -n "$release_state" ]; then
    [ "$(printf '%s\n' "$release_state" | cut -f1)" = true ] && [ "$(printf '%s\n' "$release_state" | cut -f2)" = "$sha" ] || {
        echo "release: existing $tag release conflicts with $sha" >&2; exit 1;
    }
fi
before=$(gh run list --workflow release.yml --limit 1 --json databaseId --jq '.[0].databaseId // 0')
gh workflow run release.yml --ref main -f "version=$version" -f "sha=$sha"
run_id= attempts=0
while [ -z "$run_id" ] && [ "$attempts" -lt 20 ]; do
    run_id=$(gh run list --workflow release.yml --branch main --event workflow_dispatch --limit 20 --json databaseId,headSha --jq ".[] | select(.headSha == \"$sha\" and .databaseId != $before) | .databaseId" | sed -n '1p')
    attempts=$((attempts + 1))
    [ -n "$run_id" ] || sleep 1
done
[ -n "$run_id" ] || { echo "release: dispatched workflow run was not found" >&2; exit 1; }
gh run watch "$run_id" --exit-status
