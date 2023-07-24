#!/bin/zsh
PWD=$(pwd)

if [[ -z "${XCODE_BETA_PATH}" ]]; then
    export XCODE_BETA_PATH="/Applications/Xcode-beta.app"
fi

# Remove old sim stuff
rm -rf ${XCODE_BETA_PATH}/Contents/Developer/Platforms/XROS.platform/Library/Developer/CoreSimulator/Profiles/UserInterface/XRGyroControls.simdeviceui

INSTALL_NAME_TOOL=${XCODE_BETA_PATH}/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool
VTOOL=${XCODE_BETA_PATH}/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/vtool

# Fixup monado JSON
gsed "s|REPLACE_ME|$PWD|g" openxr_monado-dev.json.template > openxr_monado-dev.json

function fixup_paths_and_sign ()
{
    which_dylib=$1
    #$INSTALL_NAME_TOOL -add_rpath $PWD $which_dylib
    codesign -s - $which_dylib --force --deep --verbose
}
#
# SimUI stuff
#

# Sign everything just in case (it complains anyway)
fixup_paths_and_sign IOUSBLib_ios_hax.dylib
fixup_paths_and_sign libusb-1.0.0.dylib
fixup_paths_and_sign libopenxr_loader.dylib
fixup_paths_and_sign libopenxr_monado.dylib
fixup_paths_and_sign libvulkan.1.dylib
fixup_paths_and_sign libSim2OpenXR.dylib
fixup_paths_and_sign libMoltenVK.dylib
fixup_paths_and_sign libcjson.1.dylib
fixup_paths_and_sign libjpeg.8.dylib
fixup_paths_and_sign libvulkan.1.dylib
fixup_paths_and_sign libx264.164.dylib
fixup_paths_and_sign XRGyroControls.simdeviceui/Contents/MacOS/XRGyroControls
codesign -s - XRGyroControls.simdeviceui --force --deep --verbose

# Copy to CoreSimulator
rm -rf ${XCODE_BETA_PATH}/Contents/Developer/Platforms/XROS.platform/Library/Developer/CoreSimulator/Profiles/UserInterface/XRGyroControls.simdeviceui
cp -r XRGyroControls.simdeviceui ${XCODE_BETA_PATH}/Contents/Developer/Platforms/XROS.platform/Library/Developer/CoreSimulator/Profiles/UserInterface/XRGyroControls.simdeviceui

# Adjust resolution
cp new_device_plists/capabilities.plist "${XCODE_BETA_PATH}/Contents/Developer/Platforms/XROS.platform/Library/Developer/CoreSimulator/Profiles/DeviceTypes/Apple Vision Pro.simdevicetype/Contents/Resources/capabilities.plist"
cp new_device_plists/profile.plist "${XCODE_BETA_PATH}/Contents/Developer/Platforms/XROS.platform/Library/Developer/CoreSimulator/Profiles/DeviceTypes/Apple Vision Pro.simdevicetype/Contents/Resources/profile.plist"

if [[ $(csrutil status) != "System Integrity Protection status: disabled." ]]; then
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo "SIP is not disabled! Disable SIP!!"
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
fi