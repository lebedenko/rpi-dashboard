#!/bin/sh
set -eu
publish_script=$1
work=$(mktemp -d "${TMPDIR:-/tmp}/rpi-dashboard-publish-test-XXXXXX")
trap 'rm -rf -- "$work"' EXIT HUP INT TERM
mkdir -p "$work/repo/scripts" "$work/bin"
cp "$publish_script" "$work/repo/scripts/release-publish.sh"
cat >"$work/repo/scripts/release-prepare.sh" <<'EOF'
#!/bin/sh
test "$1" = check
test "$2" = 0.1.3
EOF
cat >"$work/bin/gh" <<'EOF'
#!/bin/sh
printf '%s\n' "$*" >>"$GH_LOG"
case "$1 $2" in
  'api repos/{owner}/{repo}/releases')
    [ "${GH_CONFLICT:-0}" = 0 ] || printf '%s\t%s\n' false elsewhere
    ;;
  'workflow run') : >"$GH_DISPATCHED" ;;
  'run list')
    case "$*" in
      *'--workflow ci.yml'*) printf '%s\n' 77 ;;
      *'--workflow release.yml'*)
        if [ -f "$GH_DISPATCHED" ]; then printf '%s\n' 11; else printf '%s\n' 10; fi
        ;;
    esac
    ;;
  'run watch') test "$3" = 11 && test "$4" = --exit-status ;;
esac
EOF
chmod +x "$work/repo/scripts/release-prepare.sh" "$work/bin/gh"
git init -q --bare "$work/origin.git"
(cd "$work/repo" && git init -q -b main && git config user.name test && git config user.email test@example.invalid && printf '%s\n' 0.1.3 >VERSION && git add . && git commit -qm 'chore(release): prepare 0.1.3' && git remote add origin "$work/origin.git" && git push -q -u origin main)
: >"$work/gh.log"
(cd "$work/repo" && PATH="$work/bin:$PATH" GH_LOG="$work/gh.log" GH_DISPATCHED="$work/dispatched" sh scripts/release-publish.sh 0.1.3)
grep -Fq 'run list --workflow ci.yml --branch main --commit ' "$work/gh.log"
grep -Fq 'workflow run release.yml --ref main -f version=0.1.3 -f sha=' "$work/gh.log"
grep -Fq 'run watch 11 --exit-status' "$work/gh.log"

rm -f "$work/dispatched"
if (cd "$work/repo" && PATH="$work/bin:$PATH" GH_LOG="$work/gh.log" GH_DISPATCHED="$work/dispatched" GH_CONFLICT=1 sh scripts/release-publish.sh 0.1.3 >/dev/null 2>&1); then
    echo "release_publish_test: accepted a conflicting existing release" >&2
    exit 1
fi
[ ! -e "$work/dispatched" ] || { echo "release_publish_test: dispatched after a conflict" >&2; exit 1; }
