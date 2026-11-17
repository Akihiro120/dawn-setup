#include <webgpu/webgpu_cpp.h>
#include <cstdlib>
#include <iostream>

int main() {
    static constexpr auto kTimedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
    wgpu::InstanceDescriptor desc{ .requiredFeatureCount = 1, .requiredFeatures = &kTimedWaitAny };
    wgpu::Instance instance = wgpu::CreateInstance(&desc);
    if (!instance) { std::cerr << "instance creation failed\n"; return EXIT_FAILURE; }

    wgpu::Adapter adapter;
    instance.WaitAny(
        instance.RequestAdapter(nullptr, wgpu::CallbackMode::WaitAnyOnly,
            [&](wgpu::RequestAdapterStatus status, wgpu::Adapter a, wgpu::StringView message) {
                if (status != wgpu::RequestAdapterStatus::Success) {
                    std::cerr << "RequestAdapter failed: " << std::string_view(message) << "\n";
                    return;
                }
                adapter = std::move(a);
            }),
        UINT64_MAX);
    if (!adapter) return EXIT_FAILURE;

    wgpu::AdapterInfo info;
    adapter.GetInfo(&info);
    std::cout << "[C++] " << std::string_view(info.device) << " (" << std::string_view(info.description) << ")\n";
    return EXIT_SUCCESS;
}
