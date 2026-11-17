cmake_minimum_required(VERSION 3.24)
project(dawn_window_c C CXX)

set(CMAKE_C_STANDARD 11)

include(FetchContent)
set(INCLUDE_CIMGUI ON)
FetchContent_Declare(dawn_setup SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
FetchContent_MakeAvailable(dawn_setup)

add_executable(window_c main.c)
target_link_libraries(window_c PRIVATE webgpu glfw glfw_wgpu cimgui cimguizmo cglm)
