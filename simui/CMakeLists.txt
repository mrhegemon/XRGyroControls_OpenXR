cmake_minimum_required(VERSION 3.15.1)

set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0")
#set(CMAKE_OSX_SYSROOT "$ENV{XCODE_BETA_PATH}/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX14.0.sdk")
#set(CMAKE_OSX_DEPLOYMENT_TARGET "")
set(CMAKE_OSX_SYSROOT "")
set(CMAKE_Swift_LANGUAGE_VERSION "4")
project(XRGyroControls_OpenXR LANGUAGES Swift C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SYSTEM_NAME STREQUAL Windows OR CMAKE_SYSTEM_NAME STREQUAL Darwin)
  option(BUILD_SHARED_LIBS "Build shared libraries by default" YES)
endif()

INCLUDE(FindPkgConfig)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# SimulatorKit framework
add_library(SimulatorKit SimulatorKit/SimulatorKit.swift)
target_compile_options(SimulatorKit PRIVATE -I ${CMAKE_SOURCE_DIR}/CoreSimulator -target arm64-apple-macosx13.0)

# SimUI library
add_library(XRGyroControls_ObjC STATIC simui_src/SimulatorObjC.m)
set_source_files_properties(simui_src/SimulatorObjC.m PROPERTIES COMPILE_FLAGS "-fmodules -fobjc-arc -mmacosx-version-min=13.0")

add_library(XRGyroControls 
  simui_src/IndigoHID.swift simui_src/SimulatorSupport.swift 
)
target_compile_options(XRGyroControls PRIVATE -I ${CMAKE_SOURCE_DIR}/CoreSimulator -import-objc-header ${CMAKE_SOURCE_DIR}/simui_src/simui-Bridging-Header.h -target arm64-apple-macosx13.0)
target_link_libraries(XRGyroControls PRIVATE XRGyroControls_ObjC SimulatorKit /Library/Developer/PrivateFrameworks/CoreSimulator.framework/Versions/A/CoreSimulator "-framework Carbon")
