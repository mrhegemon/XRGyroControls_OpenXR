# XRGyroControls_OpenXR

## Building Instructions

Before running anything in this repository, you will want to follow the instructions at https://gist.github.com/shinyquagsire23/3c68aecd872cc7ac21c28e950245dbd2 for setting up a *normal* Quest Link on macOS environment. This includes stuff like setting up MoltenVK.

Before building, do the following:
 - Set `MONADO_BUILD_DIR` to your `monado/build` directory, ie `/Users/maxamillion/workspace/monado/build`
 - Ensure monado in `MONADO_BUILD_DIR` is compiled with `cmake .. -DXRT_ENABLE_GPL=1 -DXRT_BUILD_DRIVER_EUROC=0 -DXRT_BUILD_DRIVER_NS=0 -DXRT_BUILD_DRIVER_PSVR=0 -DXRT_HAVE_OPENCV=0 -DXRT_HAVE_XCB=0 -DXRT_HAVE_XLIB=0 -DXRT_HAVE_XRANDR=0 -DXRT_HAVE_SDL2=0  -DXRT_HAVE_VT=0 -DXRT_FEATURE_WINDOW_PEEK=0 -DXRT_BUILD_DRIVER_QWERTY=0`
 - Set `VULKAN_SDK`
 - If your Xcode-beta.app is not located at `/Applications/Xcode-beta.app`, set the `XCODE_BETA_PATH` env var to your Xcode-beta.app path.
 - Install brew dependencies: `brew install autoconf automake libtool gsed`

Once all the prerequisites are done, run `./build.sh`.

## Build Troubleshooting

- This repo has only been tested on macOS Sonoma Beta 2 and 3, Xcode 15 beta 1, and M1 macOS machines.
  - x86_64 is almost certainly broken currently -- It will need to *not* use the libusb patches and *not* use `IOUSBLib_ios_hax.dylib`. In theory it should be able to load IOUSBLib from the dylib cache w/o issues. It might also Just Work without any specific changes, dunno.
- If `libusb` fails to build, ensure you have all dependencies (especially `autoconf` and `automake`), delete the `libusb` directory, and then run `./build.sh` again
- If `libSim2OpenXR.dylib` fails to build, ensure you have all dependencies (especially `autoconf` and `automake`), delete the `libusb` directory, and then run `./build.sh` again.

## Running

Run `build_run.sh` to reset and then inject into the visionOS simulator. Run `build_inject.sh` to just inject into an existing instance.

## Known Issues

- Quest Link sometimes does not receive AADT information, and the view will look distorted. Run `./build_run.sh` to reset the simulator and then try again.
- The Quest Link video stream sometimes becomes juddery after sleeping the headset.
- The gaze ray gets stuck on the left controller when it disconnects