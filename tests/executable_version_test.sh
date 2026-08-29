#!/bin/sh

set -eu

dashboard=$1
expected=$2

[ "$(QT_QPA_PLATFORM=offscreen "$dashboard" --version)" = "rpi-dashboard $expected" ]
