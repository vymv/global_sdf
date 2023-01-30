cmake_minimum_required(VERSION 3.13)
project(gltf_sdf_test)
set(CMAKE_CXX_STANDARD 14)

add_definitions(-DPROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(gltf_sdf_test main.cpp)