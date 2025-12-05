cmake_minimum_required(VERSION 3.15)

set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)

set(VCPKG_FEATURE_FLAGS versions)
set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)
set(VCPKG_OVERLAY_TRIPLETS ${CMAKE_CURRENT_SOURCE_DIR}/platform/vcpkg/triplets)
set(VCPKG_OVERLAY_PORTS ${CMAKE_CURRENT_SOURCE_DIR}/platform/vcpkg/ports)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/modules/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

if(OSXCROSS_TARGET)
    if(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
        set(VCPKG_TARGET_TRIPLET "x64-osxcross")
        set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/platform/osxcross/osx-x64.cmake)
    elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        set(VCPKG_TARGET_TRIPLET "arm64-osxcross")
        set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/platform/osxcross/osx-arm64.cmake)
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(GODOT_LIBRARIES_SUFFIX ".macos.editor.dev.${CMAKE_OSX_ARCHITECTURES}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
    else()
        set(GODOT_LIBRARIES_SUFFIX ".macos.editor.${CMAKE_OSX_ARCHITECTURES}")
    endif()
elseif (CMAKE_SYSTEM_NAME STREQUAL "MinGW" AND UNIX)
    set(VCPKG_TARGET_TRIPLET "x64-mingw-static")
    set(VCPKG_INSTALL_OPTIONS "--allow-unsupported")
    set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/modules/vcpkg/scripts/toolchains/mingw.cmake)
    set(VCPKG_CMAKE_SYSTEM_NAME MinGW)
    set(VCPKG_TARGET_ARCHITECTURE x64)
elseif(UNIX)
    set(VCPKG_TARGET_TRIPLET "x64-linux")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(GODOT_LIBRARIES_SUFFIX ".linuxbsd.editor.dev.x86_64")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
    else()
        set(GODOT_LIBRARIES_SUFFIX ".linuxbsd.editor.x86_64")
    endif()
endif()

project(godot-vst3-host VERSION 0.1.0 LANGUAGES C CXX)

message(STATUS "VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(ENABLE_ASAN "Enable AddressSanitizer (address,undefined)" OFF)

if(APPLE)
    set(_VST3_MODULE_FILENAME "${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/vst3/hosting/module_mac.mm")
elseif(WIN32)
    set(_VST3_MODULE_FILENAME "${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/vst3/hosting/module_win32.cpp")
else()
    set(_VST3_MODULE_FILENAME "${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/vst3/hosting/module_linux.cpp")
endif()

set(SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/host/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vst3_host.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vst3_circular_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/vst3/hosting/plugprovider.cpp
    ${_VST3_MODULE_FILENAME} 
)
set(HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vst3_host.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/vst3/hosting/threadchecker.h
)

add_executable(vst3-host ${SOURCE} ${HEADER})

set_target_properties(vst3-host PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

target_include_directories(vst3-host PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/host
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(VCPKG_TRIPLET_ROOT "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
  set(VST3SDK_LIB_ROOT "${VCPKG_TRIPLET_ROOT}/debug/lib/vst3sdk/Debug")
else()
  set(VCPKG_TRIPLET_ROOT "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
  set(VST3SDK_LIB_ROOT "${VCPKG_TRIPLET_ROOT}/lib/vst3sdk/Release")
endif()

find_path(VST3SDK_INCLUDE_DIR
  NAMES pluginterfaces/vst/ivsthostapplication.h
  PATHS "${VCPKG_TRIPLET_ROOT}/include/vst3sdk"
  NO_DEFAULT_PATH
)

find_library(VST3SDK_SDK_LIB  NAMES sdk  PATHS "${VST3SDK_LIB_ROOT}" NO_DEFAULT_PATH)
find_library(VST3SDK_BASE_LIB NAMES base PATHS "${VST3SDK_LIB_ROOT}" NO_DEFAULT_PATH)
find_library(VST3SDK_PLUGINTERFACES_LIB NAMES pluginterfaces PATHS "${VST3SDK_LIB_ROOT}" NO_DEFAULT_PATH)
find_library(VST3SDK_SDK_COMMON_LIB NAMES sdk_common PATHS "${VST3SDK_LIB_ROOT}" NO_DEFAULT_PATH)
find_library(VST3SDK_SDK_HOSTING_LIB NAMES sdk_hosting PATHS "${VST3SDK_LIB_ROOT}" NO_DEFAULT_PATH)

if(NOT VST3SDK_INCLUDE_DIR OR NOT VST3SDK_SDK_LIB)
  message(FATAL_ERROR "vst3sdk not found in your local vcpkg_installed tree")
else()
  target_include_directories(vst3-host PRIVATE "${VST3SDK_INCLUDE_DIR}")
  target_link_libraries(vst3-host PRIVATE
    "${VST3SDK_SDK_HOSTING_LIB}"
    "${VST3SDK_SDK_LIB}"
    "${VST3SDK_SDK_COMMON_LIB}"
    "${VST3SDK_PLUGINTERFACES_LIB}"
    "${VST3SDK_BASE_LIB}"
  )
endif()

find_package(SndFile CONFIG REQUIRED)
target_link_libraries(vst3-host PRIVATE SndFile::sndfile)

if(ENABLE_ASAN AND UNIX)
    message(STATUS "Building with AddressSanitizer instrumentation")
    target_compile_options(vst3-host PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(vst3-host PRIVATE -fsanitize=address)
endif()
