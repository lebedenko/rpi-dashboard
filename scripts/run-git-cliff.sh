#!/bin/sh

set -eu

image='orhunp/git-cliff:2.13.1@sha256:d49216b61658fc1b10bab6c5f82dfca03b8e37278618fdc3db235d95cf3c33f5'
repo=$(git rev-parse --show-toplevel)

exec docker run --rm \
    --volume "$repo:/app:ro" \
    --workdir /app \
    "$image" "$@"
