add_executable(demo
    src/main.cpp
)

target_link_libraries(demo
    Vulkan::Vulkan
    vkmol
    glfw
    glm)

target_compile_features(demo PUBLIC cxx_std_17)

add_definitions(-DVULKAN_HPP_NO_EXCEPTIONS)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")