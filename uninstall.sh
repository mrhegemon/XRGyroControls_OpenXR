#!/bin/zsh

if [[ -z "${XCODE_BETA_PATH}" ]]; then
    export XCODE_BETA_PATH="/Applications/Xcode-beta.app"
fi

# Remove UI hax
rm -rf ${XCODE_BETA_PATH}/Contents/Developer/Platforms/XROS.platform/Library/Developer/CoreSimulator/Profiles/UserInterface/XRGyroControls.simdeviceui

# Adjust resolution back to original
cp orig_device_plists/capabilities.plist "${XCODE_BETA_PATH}/Contents/Developer/Platforms/XROS.platform/Library/Developer/CoreSimulator/Profiles/DeviceTypes/Apple Vision Pro.simdevicetype/Contents/Resources/capabilities.plist"
cp orig_device_plists/profile.plist "${XCODE_BETA_PATH}/Contents/Developer/Platforms/XROS.platform/Library/Developer/CoreSimulator/Profiles/DeviceTypes/Apple Vision Pro.simdevicetype/Contents/Resources/profile.plist"

#xcrun simctl spawn booted defaults write com.apple.RealitySimulation ShouldRecastGaze 0