
#!/bin/zsh
cmake -B build -D CMAKE_BUILD_TYPE=RelWithDebInfo -D BUILD_TESTING=YES -G Ninja -S .
ninja -C build

retVal=$?
if [ $retVal -ne 0 ]; then
    exit $retVal
fi

# Couldn't figure out how to do this with CMake, but everything gets dynamically linked so it doesn't particularly matter and we can fixup the rpaths
install_name_tool -change @rpath/libSimulatorKit.dylib  @rpath/SimulatorKit.framework/Versions/A/SimulatorKit build/libXRGyroControls.dylib
mkdir -p XRGyroControls.simdeviceui/Contents/MacOS/
cp build/libXRGyroControls.dylib XRGyroControls.simdeviceui/Contents/MacOS/XRGyroControls

# Sign just in case (it complains anyway)
codesign -s - XRGyroControls.simdeviceui --force --deep --verbose

# Copy to CoreSimulator
rm -rf /Applications/Xcode-beta.app/Contents/Developer/Platforms/XROS.platform/Library/Developer/CoreSimulator/Profiles/UserInterface/XRGyroControls.simdeviceui
cp -r XRGyroControls.simdeviceui /Applications/Xcode-beta.app/Contents/Developer/Platforms/XROS.platform/Library/Developer/CoreSimulator/Profiles/UserInterface/XRGyroControls.simdeviceui
