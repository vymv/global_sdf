cmake_minimum_required(VERSION 3.13)
project(gltf_sdf_test)
set(CMAKE_CXX_STANDARD 14)

add_definitions(-DPROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SOURCE_FILES "src/*.h" "src/*.cpp")
add_executable(gltf_sdf_test ${SOURCE_FILES})

# gltf_sdf
add_subdirectory(extern/gltf_sdf)
target_link_libraries(gltf_sdf_test PUBLIC gltf_sdf)

# volk
add_library(volk STATIC extern/volk/volk.c extern/volk/volk.h)
target_include_directories(volk PUBLIC extern/volk)
target_include_directories(volk PUBLIC extern/Vulkan-headers/include)
if (WIN32)
    target_compile_definitions(volk PRIVATE VK_USE_PLATFORM_WIN32_KHR)
endif()
target_link_libraries(gltf_sdf_test PUBLIC volk)