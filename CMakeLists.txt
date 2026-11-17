# dawn-setup: fetch this to get a prebuilt WebGPU stack.
#
#   include(FetchContent)
#   set(INCLUDE_CIMGUI ON)   # toggles: INCLUDE_DAWN, INCLUDE_GLFW, INCLUDE_IMGUI, INCLUDE_CIMGUI
#   FetchContent_Declare(dawn_setup GIT_REPOSITORY https://github.com/Akihiro120/dawn-setup.git GIT_TAG main)
#   # FetchContent_Declare(dawn_setup SOURCE_DIR /path/to/dawn-setup)       # or a local folder
#   FetchContent_MakeAvailable(dawn_setup)
#   target_link_libraries(my_app PRIVATE webgpu glfw glfw_wgpu imgui)
cmake_minimum_required(VERSION 3.24)
project(dawn_setup LANGUAGES C CXX)

add_subdirectory(vendor)
