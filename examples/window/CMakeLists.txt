cmake_minimum_required(VERSION 3.24)
project(dawn_window C CXX)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(dawn_setup SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
FetchContent_MakeAvailable(dawn_setup)

add_executable(window main.cpp)
target_link_libraries(window PRIVATE webgpu glfw glfw_wgpu imgui)
