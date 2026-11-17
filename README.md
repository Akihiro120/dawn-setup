# dawn-setup

Everything you need to start a WebGPU app in C or C++, prebuilt and ready to pull into a CMake project:
a window, a GPU device, a debug UI, 3D gizmos and a math library.

> **Platform:** macOS on Apple Silicon (arm64) only, for now.

## Quick start (C++)

Add this to your `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.24)
project(my_app C CXX)

include(FetchContent)
FetchContent_Declare(dawn_setup
    GIT_REPOSITORY https://github.com/Akihiro120/dawn-setup.git
    GIT_TAG main
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(dawn_setup)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE webgpu glfw glfw_wgpu imgui imguizmo glm)
```

Then include what you need:

```cpp
#include <webgpu/webgpu_cpp.h>   // WebGPU
#include <GLFW/glfw3.h>          // window and input
#include <glfw_wgpu.h>           // glfwCreateWGPUSurface(instance, window)
#include <imgui.h>               // debug UI
#include <ImGuizmo.h>            // 3D gizmos
#include <glm/glm.hpp>           // math
```

A complete, working program is in [`examples/window/main.cpp`](examples/window/main.cpp).

## Quick start (C)

Same as above, but turn on the C bindings for ImGui and link the C versions of each library:

```cmake
set(INCLUDE_CIMGUI ON)                  # must come before FetchContent_MakeAvailable
FetchContent_MakeAvailable(dawn_setup)

add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE webgpu glfw glfw_wgpu cimgui cimguizmo cglm)
```

```c
#include <webgpu/webgpu.h>       // WebGPU
#include <GLFW/glfw3.h>          // window and input
#include <glfw_wgpu.h>           // glfwCreateWGPUSurface(instance, window)
#include <cimgui.h>              // debug UI: igBegin, igText, ...
#include <cimgui_impl.h>         // ImGui's GLFW backend
#include <cimgui_impl_wgpu.h>    // ImGui's WebGPU backend (start with ImGui_ImplWGPU_InitC)
#include <cimguizmo.h>           // 3D gizmos: ImGuizmo_Manipulate, ...
#include <cglm/cglm.h>           // math
```

A complete, working program is in [`examples/window_c/main.c`](examples/window_c/main.c).

## What's included

| Library | What it's for | Link (C++) | Link (C) | Version |
|---|---|---|---|---|
| [Dawn](https://dawn.googlesource.com/dawn) | WebGPU, running on Metal | `webgpu` | `webgpu` | main @ `096f58f` |
| [GLFW](https://www.glfw.org) | Window and input | `glfw` | `glfw` | 3.4 |
| glfw_wgpu | Connects a GLFW window to WebGPU | `glfw_wgpu` | `glfw_wgpu` | (part of this repo) |
| [Dear ImGui](https://github.com/ocornut/imgui) | Debug UI (docking branch, with GLFW + WebGPU backends) | `imgui` | `cimgui` | 1.93.0 |
| [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) | 3D gizmos, plus sequencer and curve editors | `imguizmo` | `cimguizmo` | `105e4fd` |
| [glm](https://github.com/g-truc/glm) / [cglm](https://github.com/recp/cglm) | Vector and matrix math | `glm` | `cglm` | 1.0.3 / 0.9.7 |

Dawn and GLFW are prebuilt static libraries. Everything else is built from source as part of your project.

## Turning libraries off

Every library is on by default except the C bindings for ImGui. To change that, `set()` a toggle **before**
`FetchContent_MakeAvailable`:

```cmake
set(INCLUDE_IMGUI OFF)   # e.g. no debug UI
```

| Toggle | Default | Notes |
|---|---|---|
| `INCLUDE_DAWN` | ON | |
| `INCLUDE_GLFW` | ON | `glfw_wgpu` is added automatically when both Dawn and GLFW are on |
| `INCLUDE_IMGUI` | ON | Needs Dawn and GLFW |
| `INCLUDE_IMGUIZMO` | ON | Switches itself off when ImGui is off |
| `INCLUDE_GLM` | ON | |
| `INCLUDE_CGLM` | ON | |
| `INCLUDE_CIMGUI` | OFF | ImGui for C. Needs ImGui. `cimguizmo` is added automatically when ImGuizmo is also on |

If you turn something on without what it depends on, CMake stops and tells you what's missing.

## Things to know

- **Always list `CXX` in `project()`,** even for a C-only app. Dawn and ImGui are C++ underneath, so CMake
  needs a C++ linker.
- **Projection matrices use WebGPU's depth range (0 to 1),** not OpenGL's (-1 to 1). `glm::perspective` and
  `glm_perspective` give the right result for WebGPU automatically. To get OpenGL-style matrices, remove
  `GLM_FORCE_DEPTH_ZERO_TO_ONE` / `CGLM_FORCE_DEPTH_ZERO_TO_ONE` in `vendor/CMakeLists.txt`.
- **Turning on `INCLUDE_CIMGUI` changes how ImGui is compiled:** ImGui's deprecated functions are removed,
  and its backends get C linkage. C++ code in the same project still works, as long as it doesn't use
  deprecated ImGui functions.
- **Pin a commit for real projects.** `GIT_TAG main` follows the latest changes. Use `GIT_TAG <commit>` so
  your project always builds against the same versions.

## Examples

| Example | Language | Shows |
|---|---|---|
| [`hello`](examples/hello) | C and C++ | Prints your GPU's name. Dawn only, every other library off |
| [`window`](examples/window) | C++ | Window, ImGui panel, grid, gizmo and view cube, using glm |
| [`window_c`](examples/window_c) | C | The same window, using cimgui, cimguizmo and cglm |

Build and run one:

```sh
cd examples/window
cmake -S . -B build -G Ninja
cmake --build build
./build/window
```

## Copying the files instead of fetching

If you want the libraries inside your project, for offline builds or to edit them, use the script:

```sh
python3 setup_project.py path/to/project                     # copy everything into path/to/project/vendor
python3 setup_project.py path/to/project --only cimguizmo    # just one library, plus what it needs
python3 setup_project.py path/to/project --mode link         # symlink to this folder instead of copying
```

Then add `add_subdirectory(vendor)` to your project's `CMakeLists.txt`. The toggles in the copied
`vendor/CMakeLists.txt` are already set to match what was copied.

## Repo layout

| Path | What's in it |
|---|---|
| `CMakeLists.txt` | Entry point when fetched; it just adds `vendor/` |
| `vendor/` | Headers, prebuilt libraries, library sources, and the CMake file that defines the targets |
| `examples/` | The three examples above |
| `setup_project.py` | The copy script |

## Rebuilding Dawn

To update Dawn to a newer version:

```sh
git clone https://dawn.googlesource.com/dawn dawn-src
cmake -S dawn-src -B dawn-build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DDAWN_FETCH_DEPENDENCIES=ON -DDAWN_ENABLE_INSTALL=ON -DDAWN_BUILD_SAMPLES=OFF \
  -DDAWN_BUILD_TESTS=OFF -DTINT_BUILD_TESTS=OFF -DTINT_BUILD_CMD_TOOLS=OFF \
  -DDAWN_USE_GLFW=OFF -DDAWN_BUILD_MONOLITHIC_LIBRARY=STATIC
cmake --build dawn-build
cmake --install dawn-build --prefix dawn-install
```

The build takes roughly 20–30 minutes. Then replace the files in `vendor/webgpu`:

- `dawn-install/include/webgpu/` → `vendor/webgpu/include/webgpu/`
- `dawn-install/include/dawn/*.h` (the top-level headers only) → `vendor/webgpu/include/dawn/`
- `dawn-install/lib/libwebgpu_dawn.a` → `vendor/webgpu/lib/`

## Licenses

Each library keeps its own license, next to it in `vendor/`.
