cmake_minimum_required(VERSION 3.13)
project(global_sdf)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_definitions(-DPROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SOURCE_FILES "src/*.h" "src/*.cpp")
add_executable(global_sdf ${SOURCE_FILES})

# cgltf
add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE extern/cgltf)
target_link_libraries(global_sdf PUBLIC cgltf)

# glfw
add_subdirectory(extern/glfw EXCLUDE_FROM_ALL glfw.out)
target_link_libraries(global_sdf PRIVATE glfw)

# spark
add_subdirectory(extern/spark)
target_link_libraries(global_sdf PUBLIC spark)

# spirv_reflect
add_library(spirv_reflect STATIC extern/spirv_reflect/spirv_reflect.c extern/spirv_reflect/spirv_reflect.h)
target_include_directories(global_sdf PUBLIC extern/spirv_reflect)
target_link_libraries(global_sdf PUBLIC spirv_reflect)

# volk
add_library(volk STATIC extern/volk/volk.c extern/volk/volk.h)
target_include_directories(volk PUBLIC extern/volk)
target_include_directories(volk PUBLIC extern/Vulkan-headers/include)
if (WIN32)
    target_compile_definitions(volk PRIVATE VK_USE_PLATFORM_WIN32_KHR)
endif()
target_link_libraries(global_sdf PUBLIC volk)

add_subdirectory(extern/json)
target_include_directories(global_sdf PUBLIC extern/json/include)
target_link_libraries(global_sdf PUBLIC nlohmann_json::nlohmann_json)