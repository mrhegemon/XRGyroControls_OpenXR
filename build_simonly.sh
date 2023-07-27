#!/bin/zsh
PWD=$(pwd)

# These env vars can interfere w/ building
unset MACOSX_DEPLOYMENT_TARGET
#export MACOSX_DEPLOYMENT_TARGET="13.0"

if [[ -z "${XCODE_BETA_PATH}" ]]; then
    export XCODE_BETA_PATH="/Applications/Xcode-beta.app"
fi

# Build simui, and if it errors then abort
export MACOSX_DEPLOYMENT_TARGET="13.0"
cmake -B build_simui -D CMAKE_BUILD_TYPE=RelWithDebInfo -D BUILD_TESTING=YES -G Ninja -S simui
ninja -C build_simui -v
retVal=$?
if [ $retVal -ne 0 ]; then
    exit $retVal
fi
unset MACOSX_DEPLOYMENT_TARGET

# Remove old sim stuff
rm -rf ${XCODE_BETA_PATH}/Contents/Developer/Platforms/XROS.platform/Library/Developer/CoreSimulator/Profiles/UserInterface/XRGyroControls.simdeviceui

INSTALL_NAME_TOOL=${XCODE_BETA_PATH}/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool
VTOOL=${XCODE_BETA_PATH}/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/vtool

#
# SimUI stuff
#

# Couldn't figure out how to do this with CMake, but everything gets dynamically linked so it doesn't particularly matter and we can fixup the rpaths
mkdir -p XRGyroControls.simdeviceui/Contents/MacOS/
cp build_simui/libXRGyroControls.dylib XRGyroControls.simdeviceui/Contents/MacOS/XRGyroControls
$INSTALL_NAME_TOOL -change @rpath/libSimulatorKit.dylib  @rpath/SimulatorKit.framework/Versions/A/SimulatorKit XRGyroControls.simdeviceui/Contents/MacOS/XRGyroControls

./install.sh