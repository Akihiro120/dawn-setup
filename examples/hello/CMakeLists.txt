cmake_minimum_required(VERSION 3.24)
project(dawn_hello C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
set(INCLUDE_GLFW OFF)
set(INCLUDE_IMGUI OFF)
FetchContent_Declare(dawn_setup SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
FetchContent_MakeAvailable(dawn_setup)

add_executable(hello_c hello.c)
target_link_libraries(hello_c PRIVATE webgpu)

add_executable(hello_cpp hello.cpp)
target_link_libraries(hello_cpp PRIVATE webgpu)
