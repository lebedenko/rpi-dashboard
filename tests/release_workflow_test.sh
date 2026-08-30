#!/bin/sh

set -eu

workflow=$1

if grep -Eq '^    container:' "$workflow"; then
    echo "release_workflow_test: JavaScript actions must not run inside the Alpine job container" >&2
    exit 1
fi

grep -Fq 'fail-fast: false' "$workflow"
grep -Fq 'workflow_dispatch:' "$workflow"
if grep -Fq 'tags: ["v*"]' "$workflow"; then
    echo "release_workflow_test: release must not be tag-triggered" >&2
    exit 1
fi
grep -Fq 'cancel-in-progress: false' "$workflow"
grep -Fq 'test "$GITHUB_REF_NAME" = main' "$workflow"
grep -Fq 'test "$GITHUB_SHA" = "$REQUESTED_SHA"' "$workflow"
grep -Fq 'docker run --rm' "$workflow"
grep -Fq 'ubuntu-24.04-arm' "$workflow"
grep -Fq 'actions/checkout@' "$workflow"
grep -Fq 'actions/upload-artifact@' "$workflow"
grep -Fq 'needs: [dashboard, daemon]' "$workflow"
grep -Fq 'permissions: {contents: write}' "$workflow"
grep -Fq -- '--target "$GITHUB_SHA"' "$workflow"
grep -Fq -- '--notes-file release-notes.md' "$workflow"
if grep -Fq -- '--generate-notes' "$workflow" || grep -Fq -- '--verify-tag' "$workflow"; then
    echo "release_workflow_test: release duplicates notes or requires a pre-existing tag" >&2
    exit 1
fi
