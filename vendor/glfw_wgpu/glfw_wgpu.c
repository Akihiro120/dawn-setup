#include "glfw_wgpu.h"

#include <GLFW/glfw3.h>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#include <objc/message.h>
#include <objc/runtime.h>
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif
#include <GLFW/glfw3native.h>

#if defined(__APPLE__)
// Plain-C equivalent of:
//   NSView* view = [nsWindow contentView];
//   [view setWantsLayer:YES];
//   [view setLayer:[CAMetalLayer layer]];
//   [[view layer] setContentsScale:[nsWindow backingScaleFactor]];
static void* setup_metal_layer(GLFWwindow* window) {
    id nsWindow = (id)glfwGetCocoaWindow(window);
    id view = ((id(*)(id, SEL))objc_msgSend)(nsWindow, sel_registerName("contentView"));
    id layer = ((id(*)(Class, SEL))objc_msgSend)(objc_getClass("CAMetalLayer"), sel_registerName("layer"));
    if (!view || !layer) return NULL;

    ((void (*)(id, SEL, BOOL))objc_msgSend)(view, sel_registerName("setWantsLayer:"), YES);
    ((void (*)(id, SEL, id))objc_msgSend)(view, sel_registerName("setLayer:"), layer);
    double scale = ((double (*)(id, SEL))objc_msgSend)(nsWindow, sel_registerName("backingScaleFactor"));
    ((void (*)(id, SEL, double))objc_msgSend)(layer, sel_registerName("setContentsScale:"), scale);
    return layer;
}
#endif

WGPUSurface glfwCreateWGPUSurface(WGPUInstance instance, GLFWwindow* window) {
    if (glfwGetWindowAttrib(window, GLFW_CLIENT_API) != GLFW_NO_API) return NULL;

    WGPUSurfaceDescriptor descriptor = {0};

#if defined(_WIN32)
    WGPUSurfaceSourceWindowsHWND source = {0};
    source.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
    source.hwnd = glfwGetWin32Window(window);
    source.hinstance = GetModuleHandle(NULL);
    descriptor.nextInChain = &source.chain;
    return wgpuInstanceCreateSurface(instance, &descriptor);
#elif defined(__APPLE__)
    WGPUSurfaceSourceMetalLayer source = {0};
    source.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
    source.layer = setup_metal_layer(window);
    if (!source.layer) return NULL;
    descriptor.nextInChain = &source.chain;
    return wgpuInstanceCreateSurface(instance, &descriptor);
#elif defined(__linux__)
    if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND) {
        WGPUSurfaceSourceWaylandSurface source = {0};
        source.chain.sType = WGPUSType_SurfaceSourceWaylandSurface;
        source.display = glfwGetWaylandDisplay();
        source.surface = glfwGetWaylandWindow(window);
        descriptor.nextInChain = &source.chain;
        return wgpuInstanceCreateSurface(instance, &descriptor);
    }
    WGPUSurfaceSourceXlibWindow source = {0};
    source.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
    source.display = glfwGetX11Display();
    source.window = glfwGetX11Window(window);
    descriptor.nextInChain = &source.chain;
    return wgpuInstanceCreateSurface(instance, &descriptor);
#else
    (void)instance;
    (void)descriptor;
    return NULL;
#endif
}
