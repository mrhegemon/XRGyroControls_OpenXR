#!/bin/bash
PWD=$(pwd)

#xcrun simctl spawn booted defaults write com.apple.RealitySimulation AllowImmersiveVirtualHands 1
#xcrun simctl spawn booted defaults write com.apple.RealitySimulation ShowCursor 1
#xcrun simctl spawn booted defaults write com.apple.RealitySimulation VirtualDisplayRefreshRate 10
#xcrun simctl spawn booted defaults write com.apple.RealitySimulation isVRREnabled 0
#xcrun simctl spawn booted defaults delete com.apple.RealitySimulation SimulatedHeadset
#xcrun simctl spawn booted defaults write com.apple.RealitySimulation DebugAXPointerEnabled 1

#xcrun simctl spawn booted defaults write com.apple.RealitySimulation OverrideRenderedContentFrameRate 10
#xcrun simctl spawn booted defaults write com.apple.RealitySimulation WorstAllowedFrameTime 0.001
#xcrun simctl spawn booted defaults delete com.apple.RealitySimulation WorstAllowedFrameTime
#xcrun simctl spawn booted defaults delete com.apple.RealitySimulation isVRREnabled
xcrun simctl spawn booted defaults write com.apple.RealitySimulation ShouldRecastGaze 0

set -e
xcrun simctl spawn booted launchctl debug user/$UID/com.apple.backboardd --environment DYLD_INSERT_LIBRARIES="$PWD/libSim2OpenXR.dylib" XR_RUNTIME_JSON="$PWD/openxr_monado-dev.json" PROBER_LOG=debug XRT_LOG=debug OXR_DEBUG_GUI=0 MVK_CONFIG_RESUME_LOST_DEVICE=1 XRT_COMPOSITOR_COMPUTE=1 OXR_DEBUG_ENTRYPOINTS=0 MVK_CONFIG_FULL_IMAGE_VIEW_SWIZZLE=1 XRHAX_ROOTDIR=$PWD #LIBUSB_DEBUG=4 XRT_COMPOSITOR_LOG=debug 
xcrun simctl spawn booted launchctl kill TERM user/$UID/com.apple.backboardd

#while true
#do
#    if [[ $(pgrep backboardd) ]]; then
#        lldb -n backboardd
#        exit 0
#    fi
#done
