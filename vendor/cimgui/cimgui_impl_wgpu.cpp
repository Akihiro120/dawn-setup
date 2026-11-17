#include "cimgui_impl_wgpu.h"

#include "imgui.h"
#include "imgui_impl_wgpu.h"

extern "C" bool ImGui_ImplWGPU_InitC(const ImGui_ImplWGPU_InitInfoC* info) {
    ImGui_ImplWGPU_InitInfo init;
    init.Device = info->Device;
    if (info->NumFramesInFlight > 0) init.NumFramesInFlight = info->NumFramesInFlight;
    init.RenderTargetFormat = info->RenderTargetFormat;
    init.DepthStencilFormat = info->DepthStencilFormat;
    if (info->PipelineMultisampleState.count > 0) init.PipelineMultisampleState = info->PipelineMultisampleState;
    return ImGui_ImplWGPU_Init(&init);
}
