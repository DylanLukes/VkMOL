#ifndef VKMOL_RENDERER_SWAPCHAIN_H
#define VKMOL_RENDERER_SWAPCHAIN_H

namespace vkmol {
namespace renderer {

struct SwapchainInfo {
    unsigned int width, height;
    unsigned int imageCount;

    SwapchainInfo()
    : width(0)
    , height(0)
    , imageCount(3) {}
};

}; // namespace renderer
}; // namespace vkmol

#endif