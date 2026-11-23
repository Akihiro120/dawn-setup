# Prebuilt WebGPU stack: Dawn, GLFW, surface helper, Dear ImGui/cimgui, ImGuizmo/cimguizmo, glm/cglm.
#
# Targets (each only when enabled):
#   webgpu      Dawn (C: <webgpu/webgpu.h>, C++: <webgpu/webgpu_cpp.h>)   INCLUDE_DAWN
#   glfw        GLFW                                                       INCLUDE_GLFW
#   glfw_wgpu   glfwCreateWGPUSurface(), C and C++                         INCLUDE_DAWN + INCLUDE_GLFW
#   imgui       Dear ImGui (docking) + GLFW/WebGPU backends                INCLUDE_IMGUI
#   cimgui      C bindings for imgui and both backends                     INCLUDE_CIMGUI
#   imguizmo    ImGuizmo (gizmo, sequencer, curve/gradient/graph editors)  INCLUDE_IMGUIZMO (only with ImGui)
#   cimguizmo   C bindings for ImGuizmo                                    INCLUDE_IMGUIZMO + INCLUDE_CIMGUI
#   glm         glm, header-only C++ math                                  INCLUDE_GLM
#   cglm        cglm, header-only C math                                   INCLUDE_CGLM
#   flecs       flecs ECS, prebuilt; C and C++ API in <flecs.h>           INCLUDE_FLECS (default OFF)
#
# Set the options before FetchContent_MakeAvailable / add_subdirectory, e.g.
#   set(INCLUDE_IMGUI OFF)
#
# The consuming project must enable CXX (project(my_app C CXX)) even for C-only
# code, because Dawn and ImGui are C++.

option(INCLUDE_DAWN   "Provide the webgpu (Dawn) target"                  ON)
option(INCLUDE_GLFW   "Provide the glfw and glfw_wgpu targets"            ON)
option(INCLUDE_IMGUI  "Provide the imgui target (needs Dawn and GLFW)"    ON)
option(INCLUDE_CIMGUI "Provide the cimgui target, ImGui for C"            OFF)
# ImGuizmo switches itself off when ImGui is off.
include(CMakeDependentOption)
cmake_dependent_option(INCLUDE_IMGUIZMO "Provide the imguizmo target" ON "INCLUDE_IMGUI" OFF)
option(INCLUDE_GLM    "Provide the glm target (C++ math)"                ON)
option(INCLUDE_CGLM   "Provide the cglm target (C math)"                 ON)
option(INCLUDE_FLECS  "Provide the flecs target (ECS)"                   OFF)

set(VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Threads REQUIRED)

if(INCLUDE_CIMGUI AND NOT INCLUDE_IMGUI)
    message(FATAL_ERROR "INCLUDE_CIMGUI requires INCLUDE_IMGUI")
endif()
if(INCLUDE_IMGUI AND NOT (INCLUDE_DAWN AND INCLUDE_GLFW))
    message(FATAL_ERROR "INCLUDE_IMGUI requires INCLUDE_DAWN and INCLUDE_GLFW (for its backends)")
endif()

function(_require_dir name)
    if(NOT EXISTS ${VENDOR_DIR}/${name})
        message(FATAL_ERROR "${name} is enabled but ${VENDOR_DIR}/${name} is missing")
    endif()
endfunction()

# --- webgpu: Dawn, prebuilt static library ------------------------------------
if(INCLUDE_DAWN)
    _require_dir(webgpu)
    if(WIN32)
        set(_webgpu_lib ${VENDOR_DIR}/webgpu/lib/webgpu_dawn.lib)
    else()
        set(_webgpu_lib ${VENDOR_DIR}/webgpu/lib/libwebgpu_dawn.a)
    endif()

    add_library(webgpu STATIC IMPORTED GLOBAL)
    set_target_properties(webgpu PROPERTIES
        IMPORTED_LOCATION ${_webgpu_lib}
        IMPORTED_LINK_INTERFACE_LANGUAGES CXX
        INTERFACE_INCLUDE_DIRECTORIES ${VENDOR_DIR}/webgpu/include)
    target_link_libraries(webgpu INTERFACE Threads::Threads)
    if(APPLE)
        target_link_libraries(webgpu INTERFACE
            "-framework Foundation" "-framework CoreFoundation" "-framework Cocoa"
            "-framework IOKit" "-framework IOSurface" "-framework QuartzCore" "-framework Metal")
        # webgpu/glfw reach the link line through several targets; Apple's ld warns about that.
        include(CheckLinkerFlag)
        check_linker_flag(C "-Wl,-no_warn_duplicate_libraries" _has_no_warn_duplicate_libraries)
        if(_has_no_warn_duplicate_libraries)
            target_link_options(webgpu INTERFACE "-Wl,-no_warn_duplicate_libraries")
        endif()
    elseif(WIN32)
        target_link_libraries(webgpu INTERFACE dbghelp advapi32)
    endif()
endif()

# --- glfw: prebuilt static library --------------------------------------------
if(INCLUDE_GLFW)
    _require_dir(glfw)
    if(WIN32)
        set(_glfw_lib ${VENDOR_DIR}/glfw/lib/glfw3.lib)
    else()
        set(_glfw_lib ${VENDOR_DIR}/glfw/lib/libglfw3.a)
    endif()

    add_library(glfw STATIC IMPORTED GLOBAL)
    set_target_properties(glfw PROPERTIES
        IMPORTED_LOCATION ${_glfw_lib}
        INTERFACE_INCLUDE_DIRECTORIES ${VENDOR_DIR}/glfw/include)
    target_link_libraries(glfw INTERFACE Threads::Threads)
    if(APPLE)
        target_link_libraries(glfw INTERFACE
            "-framework Cocoa" "-framework IOKit" "-framework CoreFoundation")
    endif()
endif()

# --- glfw_wgpu: WebGPU surface from a GLFW window (C, usable from C++) --------
# #include <glfw_wgpu.h>  ->  glfwCreateWGPUSurface(instance, window)
if(INCLUDE_DAWN AND INCLUDE_GLFW)
    _require_dir(glfw_wgpu)
    add_library(glfw_wgpu STATIC ${VENDOR_DIR}/glfw_wgpu/glfw_wgpu.c)
    target_include_directories(glfw_wgpu PUBLIC ${VENDOR_DIR}/glfw_wgpu)
    target_link_libraries(glfw_wgpu PUBLIC webgpu glfw)
endif()

# --- imgui: compiled from source with the GLFW + WebGPU (Dawn) backends --------
if(INCLUDE_IMGUI)
    _require_dir(imgui)
    add_library(imgui STATIC
        ${VENDOR_DIR}/imgui/imgui.cpp
        ${VENDOR_DIR}/imgui/imgui_demo.cpp
        ${VENDOR_DIR}/imgui/imgui_draw.cpp
        ${VENDOR_DIR}/imgui/imgui_tables.cpp
        ${VENDOR_DIR}/imgui/imgui_widgets.cpp
        ${VENDOR_DIR}/imgui/backends/imgui_impl_glfw.cpp
        ${VENDOR_DIR}/imgui/backends/imgui_impl_wgpu.cpp)
    target_include_directories(imgui PUBLIC ${VENDOR_DIR}/imgui ${VENDOR_DIR}/imgui/backends)
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
    target_compile_features(imgui PUBLIC cxx_std_17)
    target_link_libraries(imgui PUBLIC webgpu glfw)
    if(APPLE)
        # The WebGPU backend's built-in surface helper includes Cocoa headers.
        set_source_files_properties(${VENDOR_DIR}/imgui/backends/imgui_impl_wgpu.cpp
            PROPERTIES COMPILE_OPTIONS "-xobjective-c++")
    endif()
    if(INCLUDE_CIMGUI)
        # cimgui's C structs are generated with obsolete functions disabled, so the C++
        # side must match for struct layouts to agree. Backends get C linkage so C
        # code can call them directly (cimgui's convention).
        target_compile_definitions(imgui PUBLIC
            IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1
            "IMGUI_IMPL_API=extern \"C\"")
    endif()
endif()

# --- cimgui: Dear ImGui for C -------------------------------------------------
# #include <cimgui.h>           core API (igBegin, igText, ...)
# #include <cimgui_impl.h>      GLFW backend (ImGui_ImplGlfw_*)
# #include <cimgui_impl_wgpu.h> WebGPU backend (ImGui_ImplWGPU_InitC, ImGui_ImplWGPU_*)
if(INCLUDE_CIMGUI)
    _require_dir(cimgui)
    add_library(cimgui STATIC
        ${VENDOR_DIR}/cimgui/cimgui.cpp
        ${VENDOR_DIR}/cimgui/cimgui_impl_wgpu.cpp)
    target_include_directories(cimgui PUBLIC ${VENDOR_DIR}/cimgui)
    # cimgui.cpp includes "./imgui/imgui.h", relative to the vendor folder.
    target_include_directories(cimgui PRIVATE ${VENDOR_DIR})
    target_link_libraries(cimgui PUBLIC imgui)
    # Only C consumers want the C struct/enum definitions; cimgui.cpp uses the C++ ones.
    target_compile_definitions(cimgui INTERFACE
        $<$<COMPILE_LANGUAGE:C>:CIMGUI_DEFINE_ENUMS_AND_STRUCTS>
        CIMGUI_USE_GLFW)
endif()

# --- imguizmo: ImGuizmo and its companion widgets -----------------------------
# #include <ImGuizmo.h>  (also ImSequencer.h, ImCurveEdit.h, ImGradient.h, GraphEditor.h, ...)
if(INCLUDE_IMGUIZMO)
    _require_dir(imguizmo)
    add_library(imguizmo STATIC
        ${VENDOR_DIR}/imguizmo/ImGuizmo.cpp
        ${VENDOR_DIR}/imguizmo/ImSequencer.cpp
        ${VENDOR_DIR}/imguizmo/ImCurveEdit.cpp
        ${VENDOR_DIR}/imguizmo/ImGradient.cpp
        ${VENDOR_DIR}/imguizmo/GraphEditor.cpp
        ${VENDOR_DIR}/imguizmo/ImVectorEditor.cpp)
    target_include_directories(imguizmo PUBLIC ${VENDOR_DIR}/imguizmo)
    target_link_libraries(imguizmo PUBLIC imgui)
endif()

# --- cimguizmo: ImGuizmo for C ------------------------------------------------
# #include <cimguizmo.h>  ->  ImGuizmo_BeginFrame, ImGuizmo_Manipulate, ...
if(INCLUDE_IMGUIZMO AND INCLUDE_CIMGUI)
    _require_dir(cimguizmo)
    add_library(cimguizmo STATIC ${VENDOR_DIR}/cimguizmo/cimguizmo.cpp)
    target_include_directories(cimguizmo PUBLIC ${VENDOR_DIR}/cimguizmo)
    target_link_libraries(cimguizmo PUBLIC imguizmo cimgui)
    # The generated wrapper also covers ImGuizmo's deprecated functions.
    if(NOT MSVC)
        target_compile_options(cimguizmo PRIVATE -Wno-deprecated-declarations)
    endif()
endif()

# --- glm / cglm: header-only math ---------------------------------------------
# Both are configured for WebGPU's clip space: depth range 0..1 instead of OpenGL's -1..1,
# so glm::perspective / glm_perspective produce matrices WebGPU can use directly.
# C++: #include <glm/glm.hpp>     C: #include <cglm/cglm.h>
if(INCLUDE_GLM)
    _require_dir(glm)
    add_library(glm INTERFACE)
    target_include_directories(glm INTERFACE ${VENDOR_DIR}/glm/include)
    target_compile_definitions(glm INTERFACE GLM_FORCE_DEPTH_ZERO_TO_ONE)
endif()

if(INCLUDE_CGLM)
    _require_dir(cglm)
    add_library(cglm INTERFACE)
    target_include_directories(cglm INTERFACE ${VENDOR_DIR}/cglm/include)
    target_compile_definitions(cglm INTERFACE CGLM_FORCE_DEPTH_ZERO_TO_ONE)
    if(UNIX AND NOT APPLE)
        target_link_libraries(cglm INTERFACE m)
    endif()
endif()

# --- flecs: ECS, prebuilt static library --------------------------------------
# #include <flecs.h>  ->  C API; from C++ the same header also gives flecs::world etc.
# Built from flecs' single-file distr/flecs.c, so only the header and library ship here.
if(INCLUDE_FLECS)
    _require_dir(flecs)
    if(WIN32)
        set(_flecs_lib ${VENDOR_DIR}/flecs/lib/flecs.lib)
    else()
        set(_flecs_lib ${VENDOR_DIR}/flecs/lib/libflecs.a)
    endif()

    add_library(flecs STATIC IMPORTED GLOBAL)
    set_target_properties(flecs PROPERTIES
        IMPORTED_LOCATION ${_flecs_lib}
        INTERFACE_INCLUDE_DIRECTORIES ${VENDOR_DIR}/flecs/include)
    # The C++ API needs C++17.
    target_compile_features(flecs INTERFACE cxx_std_17)
    target_link_libraries(flecs INTERFACE Threads::Threads)
    if(WIN32)
        target_link_libraries(flecs INTERFACE ws2_32 dbghelp)
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(flecs INTERFACE m)
    endif()
endif()
