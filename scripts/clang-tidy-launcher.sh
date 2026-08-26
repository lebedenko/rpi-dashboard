#!/usr/bin/env bash

set -euo pipefail

clang_tidy=$1
binary_directory=${2%/}
shift 2

arguments=()
before_compiler_arguments=true
for argument in "$@"; do
  if [[ $argument == -- ]]; then
    before_compiler_arguments=false
    arguments+=("$argument")
    continue
  fi
  if $before_compiler_arguments && [[ $argument == "$binary_directory"/* ]]; then
    exit 0
  fi
  if [[ $argument != -mno-direct-extern-access ]]; then
    arguments+=("$argument")
  fi
done

exec "$clang_tidy" "${arguments[@]}"
