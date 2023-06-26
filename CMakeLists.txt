cmake_minimum_required(VERSION 3.15.1)

project(XRGyroControls_OpenXR LANGUAGES Swift C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SYSTEM_NAME STREQUAL Windows OR CMAKE_SYSTEM_NAME STREQUAL Darwin)
  option(BUILD_SHARED_LIBS "Build shared libraries by default" YES)
endif()

#set(CMAKE_XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_SOURCE_DIR}/openxr-Bridging-Header.h")  

find_package(OpenGL REQUIRED)
find_package(GLEW 2.0 REQUIRED)
find_package(VULKAN REQUIRED Vulkan::Vulkan)

add_subdirectory(external/glm)
add_subdirectory(external/glfw)
INCLUDE(FindPkgConfig)
PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
PKG_SEARCH_MODULE(GLFW REQUIRED glfw3)

# SimulatorKit framework
add_library(SimulatorKit SimulatorKit/SimulatorKit.swift)
target_compile_options(SimulatorKit PRIVATE -I ${CMAKE_SOURCE_DIR}/CoreSimulator)

add_library(OpenXRCpp STATIC
  openxr_src/Buffer.cpp
  openxr_src/Buffer.h
  openxr_src/Context.cpp
  openxr_src/Context.h
  openxr_src/Headset.cpp
  openxr_src/Headset.h
  openxr_src/Main.cpp
  openxr_src/Main.h
  openxr_src/MirrorView.cpp
  openxr_src/MirrorView.h
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

# Main library
add_library(XRGyroControls 
  IndigoHID.swift SimulatorSupport.swift OpenXRWrapper.swift
)
target_compile_options(XRGyroControls PRIVATE -I ${CMAKE_SOURCE_DIR}/CoreSimulator -import-objc-header ${CMAKE_SOURCE_DIR}/openxr-Bridging-Header.h)
target_link_libraries(XRGyroControls PRIVATE SimulatorKit /Library/Developer/PrivateFrameworks/CoreSimulator.framework/Versions/A/CoreSimulator)
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

target_link_libraries(OpenXRCpp PRIVATE ${OPENGL_LIBRARIES} ${SDL2_LINK_LIBRARIES} ${GLEW_LIBRARIES} m glm ${GLFW_LINK_LIBRARIES})
target_link_libraries(XRGyroControls PRIVATE OpenXRCpp ${OPENGL_LIBRARIES} ${SDL2_LINK_LIBRARIES} ${GLEW_LIBRARIES} m glm ${GLFW_LINK_LIBRARIES})

# TODO handle this better?
target_link_libraries(XRGyroControls PRIVATE /Users/maxamillion/workspace/monado/build/src/xrt/targets/openxr/libopenxr_monado.dylib)
target_link_libraries(XRGyroControls PRIVATE /opt/homebrew/Caskroom/vulkan-sdk/1.2.162.1/macOS/lib/libvulkan.1.dylib)

target_include_directories(OpenXRCpp PRIVATE ${SDL2_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS})
