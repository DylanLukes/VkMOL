project(libvkmol VERSION 0.1)

add_library(vkmol
    src/vkmol.cpp
)

target_include_directories(vkmol PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)

target_compile_features(vkmol
    PUBLIC cxx_auto_type
    PRIVATE cxx_variadic_templates)

target_link_libraries(vkmol PRIVATE
    Vulkan::Vulkan)

install(TARGETS vkmol EXPORT VkMOLConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT VkMOLConfig DESTINATION share/VkMOL/cmake)

export(TARGETS vkmol FILE VkMOLConfig.cmake)

# TODO: Add tests.

# add_executable(testlib
#     test/testlib.cpp)

# target_link_libraries(testlib
#     lib)

# add_test(testlib testlib)