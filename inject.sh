#!/bin/bash
set -e
xcrun simctl spawn booted launchctl debug user/$UID/com.apple.backboardd --environment DYLD_INSERT_LIBRARIES="$PWD/libXRGyroControls.dylib" XR_RUNTIME_JSON="$PWD/openxr_monado-dev.json" PROBER_LOG=debug XRT_LOG=debug XRT_COMPOSITOR_LOG=debug
xcrun simctl spawn booted launchctl kill TERM user/$UID/com.apple.backboardd

#while true
#do
#    if [[ $(pgrep backboardd) ]]; then
#        lldb -n backboardd
#        exit 0
#    fi
#done
