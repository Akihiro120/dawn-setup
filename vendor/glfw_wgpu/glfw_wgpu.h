// glfw_wgpu: create a WebGPU surface for a GLFW window. Usable from C and C++.
// The window must be created with glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API).
#ifndef GLFW_WGPU_H
#define GLFW_WGPU_H

#include <webgpu/webgpu.h>

typedef struct GLFWwindow GLFWwindow;

#ifdef __cplusplus
extern "C" {
#endif

// Returns NULL on failure (unsupported platform, or the window has a GL context).
WGPUSurface glfwCreateWGPUSurface(WGPUInstance instance, GLFWwindow* window);

#ifdef __cplusplus
}
#endif

#endif  // GLFW_WGPU_H
