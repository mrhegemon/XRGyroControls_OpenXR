
#!/bin/zsh
cmake -B build -D CMAKE_BUILD_TYPE=RelWithDebInfo -D BUILD_TESTING=YES -G Ninja -S .
ninja -C build

retVal=$?
if [ $retVal -ne 0 ]; then
    exit $retVal
fi

cp build/libXRGyroControls.dylib libXRGyroControls.dylib

# Copy to CoreSimulator
rm -rf /Applications/Xcode-beta.app/Contents/Developer/Platforms/XROS.platform/Library/Developer/CoreSimulator/Profiles/UserInterface/XRGyroControls.simdeviceui
#cp -r XRGyroControls.simdeviceui /Applications/Xcode-beta.app/Contents/Developer/Platforms/XROS.platform/Library/Developer/CoreSimulator/Profiles/UserInterface/XRGyroControls.simdeviceui

# Weird lunarG Vulkan junk
#maxamillion@Maxs-MacBook-Pro lib % mv libvulkan.dylib libvulkan.dylib_
#maxamillion@Maxs-MacBook-Pro lib % mv libvulkan.1.dylib libvulkan.1.dylib_
#maxamillion@Maxs-MacBook-Pro lib % mv libvulkan.1.3.250.dylib libvulkan.1.3.250.dylib_
#maxamillion@Maxs-MacBook-Pro lib % cp libMoltenVK.dylib libvulkan.dylib
#maxamillion@Maxs-MacBook-Pro lib % cp libMoltenVK.dylib libvulkan.1.dylib
#maxamillion@Maxs-MacBook-Pro lib % cp libMoltenVK.dylib libvulkan.1.3.250.dylib


# Monado build config
# otool -L is your friend, make sure there's nothing weird
#cmake .. -DXRT_ENABLE_GPL=1 -DXRT_BUILD_DRIVER_EUROC=0 -DXRT_BUILD_DRIVER_NS=0 -DXRT_BUILD_DRIVER_PSVR=0 -DXRT_HAVE_OPENCV=0 -DXRT_HAVE_XCB=0 -DXRT_HAVE_XLIB=0 -DXRT_HAVE_XRANDR=0 -DXRT_HAVE_SDL2=0  -DXRT_HAVE_VT=0 -DXRT_FEATURE_WINDOW_PEEK=0 -DXRT_BUILD_DRIVER_QWERTY=0

function fixup_dependency ()
{
    which_dylib=$1
    vtool_src=$2
    vtool_dst=$(basename $2)
    vtool -remove-build-version macos -output $vtool_dst $vtool_src
    vtool -set-build-version xrossim 1.0 1.0 -tool ld 902.11 -output $vtool_dst $vtool_dst
    install_name_tool -change @rpath/$vtool_dst $(pwd)/$vtool_dst $which_dylib
    install_name_tool -change $vtool_src $(pwd)/$vtool_dst $which_dylib

    install_name_tool -change /System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation /System/Library/Frameworks/CoreFoundation.framework/CoreFoundation $vtool_dst
    install_name_tool -change /System/Library/Frameworks/Security.framework/Versions/A/Security /System/Library/Frameworks/Security.framework/Security $vtool_dst
    install_name_tool -change /System/Library/Frameworks/IOKit.framework/Versions/A/IOKit /System/Library/Frameworks/IOKit.framework/IOKit $vtool_dst

    install_name_tool -change /System/Library/Frameworks/Metal.framework/Versions/A/Metal /System/Library/Frameworks/Metal.framework/Metal $vtool_dst
    install_name_tool -change /System/Library/Frameworks/IOSurface.framework/Versions/A/IOSurface /System/Library/Frameworks/IOSurface.framework/IOSurface $vtool_dst
    install_name_tool -change /System/Library/Frameworks/AppKit.framework/Versions/C/AppKit /System/Library/Frameworks/AppKit.framework/AppKit $vtool_dst
    install_name_tool -change /System/Library/Frameworks/QuartzCore.framework/Versions/A/QuartzCore /System/Library/Frameworks/QuartzCore.framework/QuartzCore $vtool_dst
    install_name_tool -change /System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics /System/Library/Frameworks/CoreGraphics.framework/CoreGraphics $vtool_dst
    install_name_tool -change /System/Library/Frameworks/Foundation.framework/Versions/C/Foundation /System/Library/Frameworks/Foundation.framework/Foundation $vtool_dst

    codesign -s - $vtool_dst --force --deep --verbose
}

fixup_dependency libXRGyroControls.dylib /opt/homebrew/lib/libopenxr_loader.dylib
fixup_dependency libXRGyroControls.dylib /Users/maxamillion/workspace/monado/build/src/xrt/targets/openxr/libopenxr_monado.dylib
fixup_dependency libXRGyroControls.dylib $VULKAN_SDK/lib/libvulkan.1.dylib
fixup_dependency libXRGyroControls.dylib $VULKAN_SDK/../MoltenVK/dylib/iOS/libMoltenVK.dylib
fixup_dependency libXRGyroControls.dylib /opt/homebrew/opt/glfw/lib/libglfw.3.dylib

#fixup_dependency libopenxr_monado.dylib /opt/homebrew/opt/hidapi/lib/libhidapi.0.dylib
fixup_dependency libopenxr_monado.dylib /opt/homebrew/opt/libusb/lib/libusb-1.0.0.dylib

fixup_dependency libopenxr_monado.dylib /opt/homebrew/opt/x264/lib/libx264.164.dylib
fixup_dependency libopenxr_monado.dylib $VULKAN_SDK/lib/libvulkan.1.dylib
fixup_dependency libopenxr_monado.dylib $VULKAN_SDK/../MoltenVK/dylib/iOS/libMoltenVK.dylib
fixup_dependency libopenxr_monado.dylib /opt/homebrew/opt/cjson/lib/libcjson.1.dylib
fixup_dependency libopenxr_monado.dylib /opt/homebrew/opt/jpeg-turbo/lib/libjpeg.8.dylib

#cp $VULKAN_SDK/../MoltenVK/dylib/iOS/libMoltenVK.dylib libvulkan.1.dylib
#vtool_src=libvulkan.1.dylib
#vtool_dst=libvulkan.1.dylib
#vtool -remove-build-version macos -output $vtool_dst $vtool_src
#vtool -set-build-version xrossim 1.0 1.0 -tool ld 902.11 -output $vtool_dst $vtool_dst

codesign -s - libopenxr_loader.dylib --force --deep --verbose
codesign -s - libopenxr_monado.dylib --force --deep --verbose
codesign -s - libvulkan.1.dylib --force --deep --verbose
codesign -s - libusb-1.0.0.dylib --force --deep --verbose

# Sign just in case (it complains anyway)
codesign -s - libXRGyroControls.dylib --force --deep --verbose