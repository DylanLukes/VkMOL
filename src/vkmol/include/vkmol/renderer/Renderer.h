#ifndef VKMOL_RENDERER_RENDERER_H
#define VKMOL_RENDERER_RENDERER_H

#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>

#include "Buffer.h"
#include "ResourceHandle.h"

namespace vkmol {
namespace renderer {

using BufferHandle = ResourceHandle<Buffer>;

// note: there is only one renderer implementation, however this could be
// changed at later date to allow alternative (or mock) implementations.

struct RendererInfo {
    bool debug;
    bool trace;

    // WSI-agnostic callbacks:
    std::function<vk::SurfaceKHR(vk::Instance)>             getSurface;
    std::function<std::tuple<unsigned int, unsigned int>()> getSize;

    RendererInfo() : debug(false), trace(false) {}
};

class Renderer {
private:


public:
#pragma mark - Lifecycle

    explicit Renderer(const RendererInfo &info);

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    Renderer(Renderer &&other) = default;
    Renderer &operator=(Renderer &&other) = default;

    ~Renderer() = default;

#pragma mark - Resource Management

    BufferHandle createBuffer(BufferType type, uint32_t size, const void *contents);
};

}; // namespace renderer
}; // namespace vkmol

#endif // VKMOL_RENDERER_RENDERER_H
