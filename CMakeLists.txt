cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(vkmol VERSION 0.1)

include(GNUInstallDirs)

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)

# Apple Xcode's is missing some libc++ headers (that should be there).
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    include_directories(shims/xcode-libcxx)
endif ()

add_subdirectory(src/vkmol vkmol)
add_subdirectory(src/demo demo)

# Helps some IDEs to know what language version to use in inspections.
set(CMAKE_CXX_STANDARD 17)

option (FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." TRUE)