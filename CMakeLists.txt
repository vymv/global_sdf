cmake_minimum_required(VERSION 3.13)
project(gltf_sdf_test)
set(CMAKE_CXX_STANDARD 14)

add_definitions(-DPROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SOURCE_FILES "src/*.h" "src/*.cpp")
add_executable(gltf_sdf_test ${SOURCE_FILES})

# gltf_sdf
add_subdirectory(extern/gltf_sdf)
target_link_libraries(gltf_sdf_test PUBLIC gltf_sdf)

# glfw
add_subdirectory(extern/glfw EXCLUDE_FROM_ALL glfw.out)
target_link_libraries(gltf_sdf_test PRIVATE glfw)

# glm
add_subdirectory(extern/glm)
target_compile_definitions(glm INTERFACE GLM_ENABLE_EXPERIMENTAL=1)
target_compile_definitions(glm INTERFACE GLM_FORCE_DEPTH_ZERO_TO_ONE=1)
target_link_libraries(gltf_sdf_test PUBLIC glm)

# spirv_reflect
add_library(spirv_reflect STATIC extern/spirv_reflect/spirv_reflect.c extern/spirv_reflect/spirv_reflect.h)
target_include_directories(gltf_sdf_test PUBLIC extern/spirv_reflect)
target_link_libraries(gltf_sdf_test PUBLIC spirv_reflect)

# volk
add_library(volk STATIC extern/volk/volk.c extern/volk/volk.h)
target_include_directories(volk PUBLIC extern/volk)
target_include_directories(volk PUBLIC extern/Vulkan-headers/include)
if (WIN32)
    target_compile_definitions(volk PRIVATE VK_USE_PLATFORM_WIN32_KHR)
endif()
target_link_libraries(gltf_sdf_test PUBLIC volk)