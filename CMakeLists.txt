cmake_minimum_required(VERSION 3.15.1)

project(XRGyroControls_OpenXR LANGUAGES Swift)

if(CMAKE_SYSTEM_NAME STREQUAL Windows OR CMAKE_SYSTEM_NAME STREQUAL Darwin)
  option(BUILD_SHARED_LIBS "Build shared libraries by default" YES)
endif()

add_library(SimulatorKit SimulatorKit/SimulatorKit.swift)
target_compile_options(SimulatorKit PRIVATE -I ${CMAKE_SOURCE_DIR}/CoreSimulator)

#add_custom_target(SimulatorKit
#COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/SimulatorKit
#COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/SimulatorKit/SimulatorKit
#COMMAND swiftc -emit-module ${CMAKE_SOURCE_DIR}/SimulatorKit/SimulatorKit.swift -module-name SimulatorKit -I ${CMAKE_SOURCE_DIR}/CoreSimulator -avoid-emit-module-source-info -emit-module-path ${CMAKE_CURRENT_BINARY_DIR}/SimulatorKit/SimulatorKit -Onone
#)

add_library(XRGyroControls IndigoHID.swift SimulatorSupport.swift OpenXRWrapper.swift)
target_compile_options(XRGyroControls PRIVATE -I ${CMAKE_SOURCE_DIR}/CoreSimulator)
#target_compile_options(XRGyroControls PRIVATE -Xlinker -add_ast_path -Xlinker ${CMAKE_CURRENT_BINARY_DIR}/SimulatorKit/SimulatorKit/SimulatorKit.swiftmodule)
#add_dependencies(XRGyroControls SimulatorKit)
target_link_libraries(XRGyroControls SimulatorKit /Library/Developer/PrivateFrameworks/CoreSimulator.framework/Versions/A/CoreSimulator)
set_target_properties(XRGyroControls PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})
