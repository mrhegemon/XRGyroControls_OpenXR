#!/bin/zsh

if [[ -z "${XCODE_BETA_PATH}" ]]; then
    export XCODE_BETA_PATH="/Applications/Xcode-beta.app"
fi

xcrun simctl shutdown 72C8208F-E181-4AA4-B9CC-036B379BA1E7
pkill -9 Simulator
xcrun simctl boot 72C8208F-E181-4AA4-B9CC-036B379BA1E7
open ${XCODE_BETA_PATH}/Contents/Developer/Applications/Simulator.app
#lldb -o run /Applications/Xcode-beta.app/Contents/Developer/Applications/Simulator.app/Contents/MacOS/Simulator
#/Applications/Xcode-beta.app/Contents/Developer/Applications/Simulator.app/Contents/MacOS/Simulator
