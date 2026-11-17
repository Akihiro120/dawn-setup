#include <webgpu/webgpu.h>
#include <stdio.h>

static void on_adapter(WGPURequestAdapterStatus status, WGPUAdapter adapter,
                       WGPUStringView message, void* userdata1, void* userdata2) {
    (void)userdata2;
    if (status != WGPURequestAdapterStatus_Success) {
        fprintf(stderr, "RequestAdapter failed: %.*s\n", (int)message.length, message.data);
        return;
    }
    *(WGPUAdapter*)userdata1 = adapter;
}

int main(void) {
    WGPUInstanceFeatureName features[] = { WGPUInstanceFeatureName_TimedWaitAny };
    WGPUInstanceDescriptor desc = { .requiredFeatureCount = 1, .requiredFeatures = features };
    WGPUInstance instance = wgpuCreateInstance(&desc);
    if (!instance) { fprintf(stderr, "instance creation failed\n"); return 1; }

    WGPUAdapter adapter = NULL;
    WGPURequestAdapterCallbackInfo cb = {
        .mode = WGPUCallbackMode_WaitAnyOnly, .callback = on_adapter, .userdata1 = &adapter };
    WGPUFutureWaitInfo wait = { .future = wgpuInstanceRequestAdapter(instance, NULL, cb) };
    wgpuInstanceWaitAny(instance, 1, &wait, UINT64_MAX);
    if (!adapter) return 1;

    WGPUAdapterInfo info = {0};
    wgpuAdapterGetInfo(adapter, &info);
    printf("[C]   %.*s (%.*s)\n", (int)info.device.length, info.device.data,
           (int)info.description.length, info.description.data);

    wgpuAdapterInfoFreeMembers(info);
    wgpuAdapterRelease(adapter);
    wgpuInstanceRelease(instance);
    return 0;
}
