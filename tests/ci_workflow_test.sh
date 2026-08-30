#!/bin/sh
set -eu

workflow=$1

grep -Fq 'cmake -S daemon -B build/daemon -G Ninja -DCMAKE_BUILD_TYPE=Debug' "$workflow"
grep -Fq 'cmake --build build/daemon' "$workflow"
grep -Fq 'ctest --test-dir build/daemon --output-on-failure' "$workflow"
grep -Fq 'conventional-title:' "$workflow"
grep -Fq 'github.event.pull_request.title' "$workflow"
grep -Fq 'git log -1 --format=%s' "$workflow"

if grep -Fq 'run: task daemon-test' "$workflow"; then
  echo 'ci_workflow_test: daemon job depends on unavailable task runner' >&2
  exit 1
fi
