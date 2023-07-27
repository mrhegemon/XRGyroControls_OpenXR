#!/bin/zsh

if [[ -z "${XCODE_BETA_PATH}" ]]; then
    export XCODE_BETA_PATH="/Applications/Xcode-beta.app"
fi

SIMCTL=${XCODE_BETA_PATH}/Contents/Developer/usr/bin/simctl

UDID="unknown UDID, pls fix"
devices=$($SIMCTL list --json devices available)
if [[ "$devices" == *"72C8208F-E181-4AA4-B9CC-036B379BA1E7"* ]]; then
    UDID="72C8208F-E181-4AA4-B9CC-036B379BA1E7"
fi
if [[ "$devices" == *"7AC39665-1CAD-4FC5-BEB6-4C6269BB71BF"* ]]; then
    UDID="7AC39665-1CAD-4FC5-BEB6-4C6269BB71BF"
fi

$SIMCTL shutdown $UDID
pkill -9 Simulator
$SIMCTL boot $UDID
open ${XCODE_BETA_PATH}/Contents/Developer/Applications/Simulator.app
#lldb -o run /Applications/Xcode-beta.app/Contents/Developer/Applications/Simulator.app/Contents/MacOS/Simulator
#/Applications/Xcode-beta.app/Contents/Developer/Applications/Simulator.app/Contents/MacOS/Simulator