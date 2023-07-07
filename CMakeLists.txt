cmake_minimum_required(VERSION 3.15.1)

project(XRGyroControls_OpenXR LANGUAGES Swift C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SYSTEM_NAME STREQUAL Windows OR CMAKE_SYSTEM_NAME STREQUAL Darwin)
  option(BUILD_SHARED_LIBS "Build shared libraries by default" YES)
endif()

#set(CMAKE_OSX_SYSROOT "/Applications/Xcode-beta.app/Contents/Developer/Platforms/XRSimulator.platform/Developer/SDKs/XRSimulator1.0.sdk")
#set(CMAKE_XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_SOURCE_DIR}/openxr-Bridging-Header.h")  

find_package(OpenGL REQUIRED)
find_package(GLEW 2.0 REQUIRED)
find_package(VULKAN REQUIRED Vulkan::Vulkan)

add_subdirectory(external/glm)
INCLUDE(FindPkgConfig)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
#PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)


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
add_library(XRGyroControls 
  visionos_stereo_screenshots.m
)
#target_link_libraries(OpenXRCpp PRIVATE -framework Metal -framework CompositorServices)
target_link_options(XRGyroControls PRIVATE -isysroot "/Applications/Xcode-beta.app/Contents/Developer/Platforms/XRSimulator.platform/Developer/SDKs/XRSimulator1.0.sdk" -Wl,-U,_RETransformComponentSetWorldMatrix4x4F)

set_source_files_properties(visionos_stereo_screenshots.m PROPERTIES COMPILE_FLAGS "-fmodules -fobjc-arc -isysroot \"/Applications/Xcode-beta.app/Contents/Developer/Platforms/XRSimulator.platform/Developer/SDKs/XRSimulator1.0.sdk\"")

set_target_properties(XRGyroControls PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})

# First try openxr.pc from OpenXR SDK
include(FindPkgConfig)
pkg_search_module(OPENXR openxr)
if (OPENXR_FOUND)
  MESSAGE("OpenXR found with pkg-config")
  target_link_libraries(OpenXRCpp PRIVATE ${OPENXR_LINK_LIBRARIES})
  target_link_libraries(XRGyroControls PRIVATE ${OPENXR_LINK_LIBRARIES})
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
  target_link_libraries(XRGyroControls PRIVATE OpenXR::openxr_loader)
endif()

target_link_libraries(OpenXRCpp PRIVATE m glm)
target_link_libraries(XRGyroControls PRIVATE OpenXRCpp ${GLEW_STATIC_LIBRARIES} m glm)

# TODO handle this better?
target_link_libraries(XRGyroControls PRIVATE /Users/maxamillion/workspace/monado/build/src/xrt/targets/openxr/libopenxr_monado.dylib)
target_link_libraries(XRGyroControls PRIVATE $ENV{VULKAN_SDK}/lib/libvulkan.1.dylib /opt/homebrew/opt/libusb/lib/libusb-1.0.0.dylib)

target_include_directories(OpenXRCpp PRIVATE ${GLEW_INCLUDE_DIRS}  ${LIBUSB_INCLUDE_DIRS}  $ENV{VULKAN_SDK}/include)
