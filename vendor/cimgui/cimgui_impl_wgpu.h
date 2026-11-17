// C bindings for Dear ImGui's WebGPU backend (imgui_impl_wgpu), which cimgui doesn't generate.
// Use together with cimgui.h and cimgui_impl.h (GLFW backend).
#ifndef CIMGUI_IMPL_WGPU_H
#define CIMGUI_IMPL_WGPU_H

#include <stdbool.h>
#include <webgpu/webgpu.h>

#ifdef __cplusplus
extern "C" {
#endif

// C version of ImGui_ImplWGPU_InitInfo. Zero-initialise it, then set the fields you need;
// NumFramesInFlight = 0 means 3, PipelineMultisampleState.count = 0 means no MSAA.
typedef struct ImGui_ImplWGPU_InitInfoC {
    WGPUDevice Device;
    int NumFramesInFlight;
    WGPUTextureFormat RenderTargetFormat;
    WGPUTextureFormat DepthStencilFormat;
    WGPUMultisampleState PipelineMultisampleState;
} ImGui_ImplWGPU_InitInfoC;

bool ImGui_ImplWGPU_InitC(const ImGui_ImplWGPU_InitInfoC* info);

#ifndef __cplusplus
// The backend is compiled with IMGUI_IMPL_API=extern "C", so these are callable directly.
typedef struct ImDrawData ImDrawData;
void ImGui_ImplWGPU_Shutdown(void);
void ImGui_ImplWGPU_NewFrame(void);
void ImGui_ImplWGPU_RenderDrawData(ImDrawData* draw_data, WGPURenderPassEncoder pass_encoder);
bool ImGui_ImplWGPU_CreateDeviceObjects(void);
void ImGui_ImplWGPU_InvalidateDeviceObjects(void);
#endif

#ifdef __cplusplus
}
#endif

#endif  // CIMGUI_IMPL_WGPU_H
