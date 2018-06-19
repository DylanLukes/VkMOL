project(libvkmol VERSION 0.1)

if (${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
       add_compile_options (-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
       add_compile_options (-fcolor-diagnostics)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
       add_compile_options (-fcolor-diagnostics)
    endif ()
endif ()

add_library(vkmol
    src/renderer/Buffer.cpp
    src/renderer/Renderer.cpp
    src/renderer/ResourceContainer.cpp
    src/renderer/ResourceHandle.cpp)

# Apple Xcode's is missing some libc++ headers (that should be there).

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    include_directories(../../shims/xcode-libcxx)
endif ()

target_include_directories(vkmol PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)

target_compile_features(vkmol
    PUBLIC cxx_std_17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

target_link_libraries(vkmol Vulkan::Vulkan)

if(APPLE)
    target_link_libraries(vkmol MoltenVK)
endif()

install(TARGETS vkmol EXPORT VkMOLConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY share/ DESTINATION ${CMAKE_INSTALL_DATAROOTDIR})

install(EXPORT VkMOLConfig DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake)

export(TARGETS vkmol FILE VkMOLConfig.cmake)

# TODO : Add tests.

# add_executable(testlib
# test / testlib.cpp)

# target_link_libraries(testlib
# lib)

# add_test(testlib testlib)