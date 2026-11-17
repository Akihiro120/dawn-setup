// GLFW window + WebGPU (Dawn) surface + Dear ImGui.
#include <webgpu/webgpu_cpp.h>
#include <GLFW/glfw3.h>
#include <glfw_wgpu.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_wgpu.h>

#include <cstdlib>
#include <iostream>

static wgpu::Surface surface;
static wgpu::Device device;
static wgpu::TextureFormat surfaceFormat;

static void configureSurface(int width, int height) {
    wgpu::SurfaceConfiguration config{
        .device = device,
        .format = surfaceFormat,
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .presentMode = wgpu::PresentMode::Fifo,
    };
    surface.Configure(&config);
}

int main() {
    static constexpr auto kTimedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
    wgpu::InstanceDescriptor instanceDesc{ .requiredFeatureCount = 1, .requiredFeatures = &kTimedWaitAny };
    wgpu::Instance instance = wgpu::CreateInstance(&instanceDesc);

    if (!glfwInit()) return EXIT_FAILURE;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "dawn-setup", nullptr, nullptr);
    surface = wgpu::Surface::Acquire(glfwCreateWGPUSurface(instance.Get(), window));
    if (!surface) { std::cerr << "surface creation failed\n"; return EXIT_FAILURE; }

    wgpu::Adapter adapter;
    wgpu::RequestAdapterOptions adapterOptions{ .compatibleSurface = surface };
    instance.WaitAny(instance.RequestAdapter(&adapterOptions, wgpu::CallbackMode::WaitAnyOnly,
        [&](wgpu::RequestAdapterStatus, wgpu::Adapter a, wgpu::StringView message) {
            if (!a) std::cerr << "RequestAdapter: " << std::string_view(message) << "\n";
            adapter = std::move(a);
        }), UINT64_MAX);
    if (!adapter) return EXIT_FAILURE;

    wgpu::DeviceDescriptor deviceDesc;
    deviceDesc.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType, wgpu::StringView message) {
            std::cerr << "WebGPU error: " << std::string_view(message) << "\n";
        });
    instance.WaitAny(adapter.RequestDevice(&deviceDesc, wgpu::CallbackMode::WaitAnyOnly,
        [&](wgpu::RequestDeviceStatus, wgpu::Device d, wgpu::StringView message) {
            if (!d) std::cerr << "RequestDevice: " << std::string_view(message) << "\n";
            device = std::move(d);
        }), UINT64_MAX);
    if (!device) return EXIT_FAILURE;

    wgpu::SurfaceCapabilities caps;
    surface.GetCapabilities(adapter, &caps);
    surfaceFormat = caps.formats[0];

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    configureSurface(width, height);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOther(window, true);
    ImGui_ImplWGPU_InitInfo imguiInfo;
    imguiInfo.Device = device.Get();
    imguiInfo.RenderTargetFormat = static_cast<WGPUTextureFormat>(surfaceFormat);
    ImGui_ImplWGPU_Init(&imguiInfo);

    float clear[3] = { 0.08f, 0.08f, 0.1f };
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        instance.ProcessEvents();

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        if (w == 0 || h == 0) continue;
        if (w != width || h != height) {
            width = w;
            height = h;
            configureSurface(width, height);
        }

        wgpu::SurfaceTexture surfaceTexture;
        surface.GetCurrentTexture(&surfaceTexture);
        if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal &&
            surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal) {
            configureSurface(width, height);
            continue;
        }

        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("dawn-setup");
        ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
        ImGui::ColorEdit3("Clear color", clear);
        ImGui::End();
        ImGui::Render();

        wgpu::RenderPassColorAttachment attachment{
            .view = surfaceTexture.texture.CreateView(),
            .loadOp = wgpu::LoadOp::Clear,
            .storeOp = wgpu::StoreOp::Store,
            .clearValue = { clear[0], clear[1], clear[2], 1.0 },
        };
        wgpu::RenderPassDescriptor passDesc{ .colorAttachmentCount = 1, .colorAttachments = &attachment };

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());
        pass.End();
        wgpu::CommandBuffer commands = encoder.Finish();
        device.GetQueue().Submit(1, &commands);
        surface.Present();
    }

    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    surface = nullptr;
    device = nullptr;
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
