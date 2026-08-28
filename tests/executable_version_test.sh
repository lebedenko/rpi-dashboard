#!/bin/sh

set -eu

dashboard=$1
daemon=$2
expected=$3

[ "$(QT_QPA_PLATFORM=offscreen "$dashboard" --version)" = "rpi-dashboard $expected" ]
[ "$("$daemon" --version)" = "dashboard-daemon $expected" ]
