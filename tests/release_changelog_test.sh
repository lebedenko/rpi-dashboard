#!/bin/sh
set -eu
source_root=$1
work=$(mktemp -d "${TMPDIR:-/tmp}/rpi-dashboard-changelog-test-XXXXXX")
trap 'rm -rf -- "$work"' EXIT HUP INT TERM
repo=$work/repo
mkdir -p "$repo/scripts" "$work/bin"
cp "$source_root/scripts/changelog.sh" "$repo/scripts/"
cp "$source_root/scripts/release-prepare.sh" "$source_root/scripts/release.sh" "$repo/scripts/"
cp "$source_root/cliff.toml" "$repo/"
printf '%s\n' 0.1.0 >"$repo/VERSION"
cat >"$repo/CHANGELOG.md" <<'EOF'
# Changelog

Intro.

## [Unreleased]

## [0.1.1] - 2026-08-29

Failed release attempt.

## [0.1.0] - 2026-08-28

Published history.

[Unreleased]: https://example.invalid/v0.1.1...HEAD
[0.1.1]: https://example.invalid/v0.1.0...v0.1.1
[0.1.0]: https://example.invalid/releases/v0.1.0
EOF
cat >"$work/bin/gh" <<'EOF'
#!/bin/sh
case "$1 $2" in
  'release list') printf '%s\n' v0.1.0 ;;
  'release view') exit 1 ;;
esac
EOF
cat >"$work/fake-cliff" <<'EOF'
#!/bin/sh
printf '%s\n' "$*" | grep -F -- 'v0.1.0..HEAD'
cat <<'NOTES'
### 0 Breaking

- **Breaking:** Remove legacy protocol ([#9](https://github.com/lebedenko/rpi-dashboard/pull/9))

### Added

- Add alerts ([#10](https://github.com/lebedenko/rpi-dashboard/pull/10))

### Fixed

- Correct parser

### Changed

- Reduce allocations

### Build and dependencies

- Bump checkout

### Documentation

- Explain releases
NOTES
EOF
chmod +x "$work/bin/gh" "$work/fake-cliff"
(cd "$repo" && git init -q && git config user.name test && git config user.email test@example.invalid && git add . && git commit -qm initial && commit=$(git rev-parse HEAD) && git update-ref refs/tags/v0.1.0 "$commit" && git update-ref refs/tags/v0.1.1 "$commit" && git update-ref refs/tags/v0.1.2 "$commit")
(cd "$repo" && PATH="$work/bin:$PATH" GIT_CLIFF_RUNNER="$work/fake-cliff" RELEASE_DATE=2026-08-30 sh scripts/changelog.sh generate 0.1.3 "$work/one")
grep -Fqx '## [0.1.3] - 2026-08-30' "$work/one"
grep -Fq 'https://github.com/lebedenko/rpi-dashboard/pull/10' "$work/one"
grep -Fq 'Published history.' "$work/one"
if grep -Fq 'Failed release attempt.' "$work/one" || grep -Fq '[Unreleased]' "$work/one"; then
    echo "release_changelog_test: unpublished history was preserved" >&2
    exit 1
fi
grep -Fq '[0.1.3]: https://github.com/lebedenko/rpi-dashboard/compare/v0.1.0...v0.1.3' "$work/one"
grep -Fq '[0.1.0]: https://example.invalid/releases/v0.1.0' "$work/one"
(cd "$repo" && cp "$work/one" CHANGELOG.md && PATH="$work/bin:$PATH" GIT_CLIFF_RUNNER="$work/fake-cliff" RELEASE_DATE=2026-08-30 sh scripts/changelog.sh generate 0.1.3 "$work/two")
cmp "$work/one" "$work/two"
grep -Fq 'message = "^feat' "$source_root/cliff.toml"
grep -Fq 'group = "Fixed"' "$source_root/cliff.toml"
grep -Fq 'group = "Build and dependencies"' "$source_root/cliff.toml"
grep -Fq 'message = "^(?:test|style|chore)' "$source_root/cliff.toml"
grep -Fq 'protect_breaking_commits = true' "$source_root/cliff.toml"

(cd "$repo" && git switch -q -c release/0.1.3 && git add CHANGELOG.md && git commit -qm 'test: restore generated changelog')
if (cd "$repo" && PATH="$work/bin:$PATH" GIT_CLIFF_RUNNER="$work/fake-cliff" sh scripts/release-prepare.sh prepare malformed >/dev/null 2>&1); then
    echo "release_changelog_test: accepted a malformed version" >&2
    exit 1
fi
[ "$(sed -n '1p' "$repo/VERSION")" = 0.1.0 ]
if (cd "$repo" && PATH="$work/bin:$PATH" GIT_CLIFF_RUNNER="$work/fake-cliff" sh scripts/release-prepare.sh prepare 0.1.0 >/dev/null 2>&1); then
    echo "release_changelog_test: accepted a non-increasing version" >&2
    exit 1
fi
if (cd "$repo" && PATH="$work/bin:$PATH" GIT_CLIFF_RUNNER="$work/fake-cliff" sh scripts/release-prepare.sh prepare 0.1.2 >/dev/null 2>&1); then
    echo "release_changelog_test: accepted a reused tag" >&2
    exit 1
fi
(cd "$repo" && PATH="$work/bin:$PATH" GIT_CLIFF_RUNNER="$work/fake-cliff" RELEASE_DATE=2026-08-30 sh scripts/release-prepare.sh prepare 0.1.3)
[ "$(sed -n '1p' "$repo/VERSION")" = 0.1.3 ]
