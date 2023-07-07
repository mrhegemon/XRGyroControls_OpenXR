# XRGyroControls_OpenXR

Before running anything in this repository, you will want to follow the instructions at https://gist.github.com/shinyquagsire23/3c68aecd872cc7ac21c28e950245dbd2 for setting up a *normal* Quest Link on macOS environment. This includes stuff like setting up MoltenVK.

Before building, do the following:
 - Set `MONADO_BUILD_DIR` to your `monado/build` directory, ie `/Users/maxamillion/workspace/monado/build`
 - Ensure monado in `MONADO_BUILD_DIR` is compiled with `cmake .. -DXRT_ENABLE_GPL=1 -DXRT_BUILD_DRIVER_EUROC=0 -DXRT_BUILD_DRIVER_NS=0 -DXRT_BUILD_DRIVER_PSVR=0 -DXRT_HAVE_OPENCV=0 -DXRT_HAVE_XCB=0 -DXRT_HAVE_XLIB=0 -DXRT_HAVE_XRANDR=0 -DXRT_HAVE_SDL2=0  -DXRT_HAVE_VT=0 -DXRT_FEATURE_WINDOW_PEEK=0 -DXRT_BUILD_DRIVER_QWERTY=0`
 - Set `VULKAN_SDK`

Run `build_run.sh` to reset and then inject into the visionOS simulator. Run `build_inject.sh` to just inject into an existing instance.

Running list of brew dependencies I remember: `brew install gsed`

See `build.sh` for a list of my sins.