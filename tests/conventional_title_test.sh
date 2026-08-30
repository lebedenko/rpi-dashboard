#!/bin/sh
set -eu
validator=$1
sh "$validator" 'feat(weather): add forecast alerts (#42)'
sh "$validator" 'fix!: reject stale snapshots'
sh "$validator" 'chore(deps): bump Qt image'
if sh "$validator" 'Add forecast alerts' >/dev/null 2>&1; then
    echo "conventional_title_test: accepted a non-Conventional title" >&2
    exit 1
fi
