#ifndef VKMOL_RENDERER_SWAPCHAIN_H
#define VKMOL_RENDERER_SWAPCHAIN_H

namespace vkmol {
namespace renderer {

struct SwapchainInfo {
    unsigned int width, height;
    unsigned int imageCount;

    SwapchainInfo();
};

}; // namespace renderer
}; // namespace vkmol

#endif