cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(vkmol VERSION 0.1)

include(GNUInstallDirs)

find_package(Vulkan REQUIRED)

add_subdirectory(vkmol)
add_subdirectory(demo)