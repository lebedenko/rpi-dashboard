#!/bin/sh

set -eu

workflow=$1

if grep -Eq '^    container:' "$workflow"; then
    echo "release_workflow_test: JavaScript actions must not run inside the Alpine job container" >&2
    exit 1
fi

grep -Fq 'fail-fast: false' "$workflow"
grep -Fq 'docker run --rm' "$workflow"
grep -Fq 'ubuntu-24.04-arm' "$workflow"
grep -Fq 'actions/checkout@' "$workflow"
grep -Fq 'actions/upload-artifact@' "$workflow"
