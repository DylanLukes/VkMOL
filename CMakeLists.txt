cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(vkmol VERSION 0.1)

include(GNUInstallDirs)

if(NOT DEFINED ENV{CCP4})
    message(FATAL_ERROR "Please set $CCP4 (see README/cmake cache).")
endif()

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)

set(CCP4_FIND_COMPONENTS
    clipper-core
    clipper-ccp4
    clipper-mmdb
    mmdb2)
set(ccp4_DIR $ENV{CCP4}/share/ccp4/)
find_package(ccp4 REQUIRED CONFIG NO_DEFAULT_PATH)

add_subdirectory(src/vkmol vkmol)
add_subdirectory(src/demo demo)

# Helps some IDEs to know what language version to use in inspections.
set(CMAKE_CXX_STANDARD 17)

option (FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." TRUE)