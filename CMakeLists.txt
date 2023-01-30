cmake_minimum_required(VERSION 3.13)
project(gltf_sdf_test)
set(CMAKE_CXX_STANDARD 14)

add_definitions(-DPROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(gltf_sdf_test main.cpp)

# gltf_sdf
add_subdirectory(gltf_sdf)
target_link_libraries(gltf_sdf_test PUBLIC gltf_sdf)