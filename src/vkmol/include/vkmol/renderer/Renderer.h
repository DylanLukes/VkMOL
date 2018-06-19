#ifndef VKMOL_RENDERER_RENDERER_H
#define VKMOL_RENDERER_RENDERER_H

#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>

namespace vkmol {
namespace renderer {

// note: there is only one renderer implementation, however this could be
// changed at later date to allow alternative (or mock) implementations.

struct RendererInfo {
    bool debug;
    bool trace;

    // WSI-agnostic callbacks:
    std::function<vk::SurfaceKHR(vk::Instance)> getSurface;
    std::function<std::tuple<unsigned int, unsigned int>()> getSize;

    RendererInfo()
    : debug(false)
    , trace(false) {}
};

class RendererImpl;

class Renderer {
private:
    std::unique_ptr<RendererImpl> impl;

public:
    explicit Renderer(const RendererInfo &info);

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    Renderer(Renderer &&other);
    Renderer &operator=(Renderer &&other);

    ~Renderer();
};

}; // namespace renderer
}; // namespace vkmol

#endif //VKMOL_RENDERER_RENDERER_H
