#!/bin/bash
PWD=$(pwd)

if [[ -z "${XCODE_BETA_PATH}" ]]; then
    export XCODE_BETA_PATH="/Applications/Xcode-beta.app"
fi
SIMCTL=${XCODE_BETA_PATH}/Contents/Developer/usr/bin/simctl

$SIMCTL spawn booted defaults write com.apple.RealitySystemSupport EnableReclinedMode 1
$SIMCTL spawn booted defaults write com.apple.RealityEnvironment activeSyntheticEnvironment MuseumDay #KitchenDay    KitchenNight    LivingRoomDay   LivingRoomNight MuseumDay   MuseumNight
#$SIMCTL spawn booted defaults delete com.apple.RealitySimulation AllowImmersiveVirtualHands

#$SIMCTL spawn booted defaults write com.apple.RealitySimulation AllowImmersiveVirtualHands 1
$SIMCTL spawn booted defaults write com.apple.RealitySimulation ShowCursor 1
#$SIMCTL spawn booted defaults write com.apple.RealitySimulation VirtualDisplayRefreshRate 10
#$SIMCTL spawn booted defaults write com.apple.RealitySimulation isVRREnabled 0
#$SIMCTL spawn booted defaults delete com.apple.RealitySimulation SimulatedHeadset
#$SIMCTL spawn booted defaults write com.apple.RealitySimulation DebugAXPointerEnabled 1

#$SIMCTL spawn booted defaults write com.apple.RealitySimulation OverrideRenderedContentFrameRate 10
#$SIMCTL spawn booted defaults write com.apple.RealitySimulation WorstAllowedFrameTime 0.001
#$SIMCTL spawn booted defaults delete com.apple.RealitySimulation WorstAllowedFrameTime
#$SIMCTL spawn booted defaults delete com.apple.RealitySimulation isVRREnabled
$SIMCTL spawn booted defaults write com.apple.RealitySimulation ShouldRecastGaze 0

set -e
$SIMCTL spawn booted launchctl debug user/$UID/com.apple.backboardd --environment DYLD_INSERT_LIBRARIES="$PWD/libSim2OpenXR.dylib" XR_RUNTIME_JSON="$PWD/openxr_monado-dev.json" PROBER_LOG=debug XRT_LOG=debug OXR_DEBUG_GUI=0 MVK_CONFIG_RESUME_LOST_DEVICE=1 XRT_COMPOSITOR_COMPUTE=1 OXR_DEBUG_ENTRYPOINTS=0 MVK_CONFIG_FULL_IMAGE_VIEW_SWIZZLE=1 XRHAX_ROOTDIR=$PWD XRT_MESH_SIZE=64 #LIBUSB_DEBUG=4 XRT_COMPOSITOR_LOG=debug 
$SIMCTL spawn booted launchctl kill TERM user/$UID/com.apple.backboardd

#while true
#do
#    if [[ $(pgrep backboardd) ]]; then
#        lldb -n backboardd
#        exit 0
#    fi
#done
