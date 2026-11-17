// GLFW window + WebGPU (Dawn) surface + Dear ImGui, in plain C (via cimgui).
#include <webgpu/webgpu.h>
#include <GLFW/glfw3.h>
#include <glfw_wgpu.h>
#include <cimgui.h>
#include <cimgui_impl.h>
#include <cimgui_impl_wgpu.h>

#include <stdio.h>
#include <stdlib.h>

static WGPUSurface surface;
static WGPUDevice device;
static WGPUTextureFormat surfaceFormat;

static void on_adapter(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message,
                       void* userdata1, void* userdata2) {
    (void)status; (void)userdata2;
    if (!adapter) fprintf(stderr, "RequestAdapter: %.*s\n", (int)message.length, message.data);
    *(WGPUAdapter*)userdata1 = adapter;
}

static void on_device(WGPURequestDeviceStatus status, WGPUDevice dev, WGPUStringView message,
                      void* userdata1, void* userdata2) {
    (void)status; (void)userdata2;
    if (!dev) fprintf(stderr, "RequestDevice: %.*s\n", (int)message.length, message.data);
    *(WGPUDevice*)userdata1 = dev;
}

static void on_error(WGPUDevice const* dev, WGPUErrorType type, WGPUStringView message,
                     void* userdata1, void* userdata2) {
    (void)dev; (void)type; (void)userdata1; (void)userdata2;
    fprintf(stderr, "WebGPU error: %.*s\n", (int)message.length, message.data);
}

static void configure_surface(int width, int height) {
    WGPUSurfaceConfiguration config = {
        .device = device,
        .format = surfaceFormat,
        .usage = WGPUTextureUsage_RenderAttachment,
        .width = (uint32_t)width,
        .height = (uint32_t)height,
        .alphaMode = WGPUCompositeAlphaMode_Auto,
        .presentMode = WGPUPresentMode_Fifo,
    };
    wgpuSurfaceConfigure(surface, &config);
}

int main(void) {
    WGPUInstanceFeatureName features[] = { WGPUInstanceFeatureName_TimedWaitAny };
    WGPUInstanceDescriptor instanceDesc = { .requiredFeatureCount = 1, .requiredFeatures = features };
    WGPUInstance instance = wgpuCreateInstance(&instanceDesc);

    if (!glfwInit()) return EXIT_FAILURE;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "dawn-setup (C)", NULL, NULL);
    surface = glfwCreateWGPUSurface(instance, window);
    if (!surface) { fprintf(stderr, "surface creation failed\n"); return EXIT_FAILURE; }

    WGPUAdapter adapter = NULL;
    WGPURequestAdapterOptions adapterOptions = { .compatibleSurface = surface };
    WGPUFutureWaitInfo wait = { .future = wgpuInstanceRequestAdapter(instance, &adapterOptions,
        (WGPURequestAdapterCallbackInfo){ .mode = WGPUCallbackMode_WaitAnyOnly, .callback = on_adapter, .userdata1 = &adapter }) };
    wgpuInstanceWaitAny(instance, 1, &wait, UINT64_MAX);
    if (!adapter) return EXIT_FAILURE;

    WGPUDeviceDescriptor deviceDesc = {
        .uncapturedErrorCallbackInfo = { .callback = on_error },
    };
    wait = (WGPUFutureWaitInfo){ .future = wgpuAdapterRequestDevice(adapter, &deviceDesc,
        (WGPURequestDeviceCallbackInfo){ .mode = WGPUCallbackMode_WaitAnyOnly, .callback = on_device, .userdata1 = &device }) };
    wgpuInstanceWaitAny(instance, 1, &wait, UINT64_MAX);
    if (!device) return EXIT_FAILURE;
    WGPUQueue queue = wgpuDeviceGetQueue(device);

    WGPUSurfaceCapabilities caps = {0};
    wgpuSurfaceGetCapabilities(surface, adapter, &caps);
    surfaceFormat = caps.formats[0];
    wgpuSurfaceCapabilitiesFreeMembers(caps);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    configure_surface(width, height);

    igCreateContext(NULL);
    ImGui_ImplGlfw_InitForOther(window, true);
    ImGui_ImplWGPU_InitInfoC imguiInfo = { .Device = device, .RenderTargetFormat = surfaceFormat };
    ImGui_ImplWGPU_InitC(&imguiInfo);

    float clear[3] = { 0.08f, 0.08f, 0.1f };
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        wgpuInstanceProcessEvents(instance);

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        if (w == 0 || h == 0) continue;
        if (w != width || h != height) {
            width = w;
            height = h;
            configure_surface(width, height);
        }

        WGPUSurfaceTexture surfaceTexture = {0};
        wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
            surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
            configure_surface(width, height);
            continue;
        }

        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        igNewFrame();
        igBegin("dawn-setup (C)", NULL, 0);
        igText("%.1f FPS", igGetIO_Nil()->Framerate);
        igColorEdit3("Clear color", clear, 0);
        igEnd();
        igRender();

        WGPUTextureView view = wgpuTextureCreateView(surfaceTexture.texture, NULL);
        WGPURenderPassColorAttachment attachment = {
            .view = view,
            .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .clearValue = { clear[0], clear[1], clear[2], 1.0 },
        };
        WGPURenderPassDescriptor passDesc = { .colorAttachmentCount = 1, .colorAttachments = &attachment };

        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, NULL);
        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &passDesc);
        ImGui_ImplWGPU_RenderDrawData(igGetDrawData(), pass);
        wgpuRenderPassEncoderEnd(pass);
        WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, NULL);
        wgpuQueueSubmit(queue, 1, &commands);
        wgpuSurfacePresent(surface);

        wgpuCommandBufferRelease(commands);
        wgpuRenderPassEncoderRelease(pass);
        wgpuCommandEncoderRelease(encoder);
        wgpuTextureViewRelease(view);
        wgpuTextureRelease(surfaceTexture.texture);
    }

    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    igDestroyContext(NULL);
    wgpuQueueRelease(queue);
    wgpuSurfaceRelease(surface);
    wgpuDeviceRelease(device);
    wgpuAdapterRelease(adapter);
    wgpuInstanceRelease(instance);
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
