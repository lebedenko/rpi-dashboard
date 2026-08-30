#!/bin/sh
set -eu
subject=${1:-}
[ -n "$subject" ] || { echo "conventional: commit or PR title is required" >&2; exit 2; }
printf '%s\n' "$subject" | grep -Eq '^(feat|fix|perf|refactor|build|ci|docs|test|style|chore)(\([a-z0-9][a-z0-9._/-]*\))?!?: .+' || {
    echo "conventional: title must use a supported Conventional Commit type: $subject" >&2
    exit 1
}
