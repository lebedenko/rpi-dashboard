#!/usr/bin/env bash

set -euo pipefail

clang_tidy=$1
shift

arguments=()
for argument in "$@"; do
  if [[ $argument != -mno-direct-extern-access ]]; then
    arguments+=("$argument")
  fi
done

exec "$clang_tidy" "${arguments[@]}"
