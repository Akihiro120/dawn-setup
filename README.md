# dawn-setup

A prebuilt WebGPU stack for macOS (arm64, Release), pulled into projects with CMake `FetchContent`.

| Target      | Toggle (default)        | What |
|-------------|-------------------------|------|
| `webgpu`    | `INCLUDE_DAWN` (ON)     | Dawn @ 096f58f98, static, Metal. C: `<webgpu/webgpu.h>`, C++: `<webgpu/webgpu_cpp.h>` |
| `glfw`      | `INCLUDE_GLFW` (ON)     | GLFW 3.4, static |
| `glfw_wgpu` | Dawn + GLFW both on      | `glfwCreateWGPUSurface(instance, window)`, C and C++ |
| `imgui`     | `INCLUDE_IMGUI` (ON)    | Dear ImGui 1.93.0 (docking) + GLFW/WebGPU backends; needs Dawn + GLFW |
| `cimgui`    | `INCLUDE_CIMGUI` (OFF)  | ImGui for C (cimgui @ 2a6a93f + WebGPU backend wrapper); needs ImGui |

## Use it

```cmake
cmake_minimum_required(VERSION 3.24)
project(my_app C CXX)            # CXX is needed even for C-only apps (Dawn/ImGui are C++)

include(FetchContent)
# Toggles go before MakeAvailable:
# set(INCLUDE_IMGUI OFF)
# set(INCLUDE_CIMGUI ON)
FetchContent_Declare(dawn_setup SOURCE_DIR /Users/akihiro/Desktop/dawn-setup)
# or, once pushed:  FetchContent_Declare(dawn_setup GIT_REPOSITORY <url> GIT_TAG main)
FetchContent_MakeAvailable(dawn_setup)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE webgpu glfw glfw_wgpu imgui)
```

### ImGui from C

`set(INCLUDE_CIMGUI ON)`, link `cimgui`, then:

```c
#include <cimgui.h>            // igBegin, igText, ...
#include <cimgui_impl.h>       // ImGui_ImplGlfw_*
#include <cimgui_impl_wgpu.h>  // ImGui_ImplWGPU_InitC + ImGui_ImplWGPU_*
```

Turning cimgui on compiles ImGui with `IMGUI_DISABLE_OBSOLETE_FUNCTIONS` and gives the backends C
linkage (cimgui's convention). C++ code using `imgui` in the same project is unaffected apart from
obsolete APIs being unavailable.

## Self-contained copy (offline route)

    python3 setup_project.py <project>                   # copy into <project>/vendor
    python3 setup_project.py <project> --only cimgui     # pulls in its dependencies
    python3 setup_project.py <project> --mode link       # symlink instead of copying

then `add_subdirectory(vendor)`. The copied `vendor/CMakeLists.txt` has the toggles preset to what was copied.

## Layout

- `CMakeLists.txt` entry point for FetchContent (just `add_subdirectory(vendor)`)
- `vendor/` headers, prebuilt libs, sources, and the CMake that defines the targets
- `examples/` `hello` (C + C++, Dawn only), `window` (C++ + ImGui), `window_c` (C + cimgui)
- `_build/` sources and build trees (git-ignored, 1.5 GB); delete it if you won't rebuild

## Rebuilding Dawn

    cmake -S _build/dawn-src -B _build/dawn-build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DDAWN_FETCH_DEPENDENCIES=ON -DDAWN_ENABLE_INSTALL=ON -DDAWN_BUILD_SAMPLES=OFF \
      -DDAWN_BUILD_TESTS=OFF -DTINT_BUILD_TESTS=OFF -DTINT_BUILD_CMD_TOOLS=OFF \
      -DDAWN_USE_GLFW=OFF -DDAWN_BUILD_MONOLITHIC_LIBRARY=STATIC
    cmake --build _build/dawn-build && cmake --install _build/dawn-build --prefix _build/dawn-install

then copy `_build/dawn-install/include/{webgpu,dawn/*.h}` and `lib/libwebgpu_dawn.a` into `vendor/webgpu`.
