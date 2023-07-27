cmake_minimum_required(VERSION 3.15.1)

#set(CMAKE_OSX_DEPLOYMENT_TARGET "13.4")
#set(CMAKE_OSX_SYSROOT "$ENV{XCODE_BETA_PATH}/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX14.0.sdk")
set(CMAKE_OSX_DEPLOYMENT_TARGET "")
set(CMAKE_OSX_SYSROOT "")
project(XRGyroControls_OpenXR LANGUAGES Swift C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SYSTEM_NAME STREQUAL Windows OR CMAKE_SYSTEM_NAME STREQUAL Darwin)
  option(BUILD_SHARED_LIBS "Build shared libraries by default" YES)
endif()

find_package(GLEW 2.0 REQUIRED)
find_package(VULKAN REQUIRED Vulkan::Vulkan)

add_subdirectory(external/glm)
INCLUDE(FindPkgConfig)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
#PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(OpenXRCpp STATIC
  openxr_src/Buffer.cpp
  openxr_src/Buffer.h
  openxr_src/Context.cpp
  openxr_src/Context.h
  openxr_src/Headset.cpp
  openxr_src/Headset.h
  openxr_src/Main.cpp
  openxr_src/Main.h
  openxr_src/Pipeline.cpp
  openxr_src/Pipeline.h
  openxr_src/Renderer.cpp
  openxr_src/Renderer.h
  openxr_src/RenderProcess.cpp
  openxr_src/RenderProcess.h
  openxr_src/RenderTarget.cpp
  openxr_src/RenderTarget.h
  openxr_src/Util.cpp
  openxr_src/Util.h
)

# dummy library
add_library(Dummy 
  dummy.m
)

# Main library
add_library(Sim2OpenXR 
  visionos_stereo_screenshots.m
)
#target_link_libraries(OpenXRCpp PRIVATE -framework Metal -framework CompositorServices)
target_link_options(Sim2OpenXR PRIVATE -Wl,-U,_RETransformComponentSetWorldMatrix4x4F, -Wl,-U,_RCPAnchorDefinitionComponentInitWithHand -Wl,-U,_RERenderFrameSettingsSetTotalTime -Wl,-U,_RERenderManagerWaitForFramePacing, -Wl,-U,_RSIsRunningOnSimulator -Wl,-U,_RCPHIDEventGetSelectionRay, -Wl,-U,_RFAnchorPtrGetIndexTipTransform, -Wl,-U,___extractTargetTimes -Wl,-U,_cp_drawable_encode_present -Wl,-U,_cp_drawable_get_color_texture -Wl,-U,_cp_drawable_get_depth_texture -Wl,-U,_cp_drawable_get_texture_count -Wl,-U,_cp_drawable_get_view -Wl,-U,_cp_drawable_get_view_count -Wl,-U,_cp_frame_query_drawable -Wl,-U,_cp_frame_timing_get_presentation_time -Wl,-U,_cp_frame_timing_get_rendering_deadline -Wl,-U,_cp_layer_properties_get_view_count -Wl,-U,_cp_view_get_view_texture_map -Wl,-U,_cp_layer_get_state)

# HACK: change platform to macOS from visionOS for now
add_custom_command(TARGET Sim2OpenXR
                   PRE_LINK
                   COMMAND bash -c "$ENV{XCODE_BETA_PATH}/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/vtool -remove-build-version 12 -output $<TARGET_OBJECTS:Sim2OpenXR> $<TARGET_OBJECTS:Sim2OpenXR>"
                   )

set_source_files_properties(visionos_stereo_screenshots.m PROPERTIES COMPILE_FLAGS "-fmodules -fobjc-arc -isysroot \"$ENV{XCODE_BETA_PATH}/Contents/Developer/Platforms/XRSimulator.platform/Developer/SDKs/XRSimulator1.0.sdk\"")

set_target_properties(Sim2OpenXR PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})

# First try openxr.pc from OpenXR SDK
include(FindPkgConfig)
pkg_search_module(OPENXR openxr)
if (OPENXR_FOUND)
  MESSAGE("OpenXR found with pkg-config")
  target_link_libraries(OpenXRCpp PRIVATE ${OPENXR_LINK_LIBRARIES})
  target_link_libraries(Sim2OpenXR PRIVATE ${OPENXR_LINK_LIBRARIES})
  MESSAGE("${OPENXR_LIBRARIES}")

# Second, try OpenXRConfig.cmake from OpenXR SDK
else()
  MESSAGE("OpenXR not found with pkg-config, trying cmake script")

  # current issue in upstream OpenXR cmake files requires us to find Threads on our own
  find_package(Threads REQUIRED)

  find_package(OpenXR REQUIRED)
  if (NOT OpenXR_FOUND)
     MESSAGE(FATAL_ERROR "OpenXR not found!")
  endif()

  target_include_directories(OpenXRCpp PRIVATE ${OpenXR_INCLUDE_DIR})
  target_link_libraries(OpenXRCpp PRIVATE OpenXR::openxr_loader)
  target_link_libraries(Sim2OpenXR PRIVATE OpenXR::openxr_loader)
endif()

target_link_libraries(OpenXRCpp PRIVATE m glm)
target_link_libraries(Sim2OpenXR PRIVATE OpenXRCpp ${GLEW_STATIC_LIBRARIES} m glm)

# TODO handle this better?
target_link_libraries(Sim2OpenXR PRIVATE $ENV{MONADO_BUILD_DIR}/src/xrt/targets/openxr/libopenxr_monado.dylib)
target_link_libraries(Sim2OpenXR PRIVATE $ENV{VULKAN_SDK}/lib/libvulkan.1.dylib /opt/homebrew/opt/libusb/lib/libusb-1.0.0.dylib Threads::Threads)

target_include_directories(OpenXRCpp PRIVATE ${GLEW_INCLUDE_DIRS}  ${LIBUSB_INCLUDE_DIRS}  $ENV{VULKAN_SDK}/include)