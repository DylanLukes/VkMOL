/*
  Copyright 2018, Dylan Lukes, University of Pittsburgh

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef VKMOL_RENDERER_RENDERER_H
#define VKMOL_RENDERER_RENDERER_H

#include "Buffer.h"
#include "Resource.h"

#include <functional>
#include <unordered_set>

#include <vulkan/vulkan.hpp>

namespace vkmol {
namespace renderer {

struct RendererWSIDelegate {
    using InstanceExtensionsCallback = std::function<std::vector<const char *>()>;

    std::function<std::vector<const char *>()>  getInstanceExtensions;
    std::function<vk::SurfaceKHR(vk::Instance)> getSurface;
    std::function<std::tuple<int, int>()>       getWindowSize;
    std::function<std::tuple<int, int>()>       getFramebufferSize;
};

struct RendererInfo {
    bool debug = false;
    bool trace = false;

    size_t ringBufferSize = 1024;

    std::string               appName = "Untitled App";
    std::tuple<int, int, int> appVersion = {1, 0, 0};

    RendererWSIDelegate delegate;
};

typedef ResourceHandle<Buffer> BufferHandle;

class Renderer {
private:
    // todo: std::vector<Frame> frames;

    RendererWSIDelegate delegate;

    ResourceContainer<Buffer> buffers;
    // todo: ... other resource containers

    vk::Instance                       instance;
    vk::DebugReportCallbackEXT         debugCallback;
    vk::PhysicalDevice                 physicalDevice;
    vk::PhysicalDeviceProperties       deviceProperties;
    vk::PhysicalDeviceFeatures         deviceFeatures;
    vk::Device                         device;
    vk::SurfaceKHR                     surface;
    vk::PhysicalDeviceMemoryProperties memoryProperties;
    uint32_t                           graphicsQueueIndex = 0;
    uint32_t                           transferQueueIndex = 0;

    std::unordered_set<vk::Format>         surfaceFormats;
    vk::SurfaceCapabilitiesKHR             surfaceCapabilities;
    std::unordered_set<vk::PresentModeKHR> surfacePresentModes;
    vk::SwapchainKHR                       swapchain;
    // vk::PipelineCache                      pipelineCache;
    vk::Queue graphicsQueue;
    vk::Queue transferQueue;

    vk::Semaphore acquireSemaphore;
    vk::Semaphore finishedSemaphore;

    VmaAllocator  allocator;
    VmaAllocation ringBufferMemory;
    vk::Buffer    ringBuffer;
    size_t        ringBufferSize = 0;
    size_t        ringBufferOffset = 0;
    uint8_t *     persistentMapping = nullptr;

    // Synchronized up to this ringbuffer index (bookkeeping).
    uint32_t lastSyncedRingBufferIndex;

    // The command pool for transfers is persistent, whereas we otherwise
    // use a distinct ephemeral command pool per frame.
    vk::CommandPool transferCommandPool;
    //    std::vector<UploadOp> uploads;

    bool debugMarkers = false;

    // These resources are cleaned out every frame.
    std::unordered_set<Resource> graveyard;

    bool isSwapchainDirty = true;

    uint32_t currentFrame = 0;
    uint32_t lastSyncedFrame = 0;

    unsigned long uboAlignment = 0;
    unsigned long ssboAlignment = 0;

    struct ResourceDeleter final {

        Renderer *renderer;

        ResourceDeleter(Renderer *renderer) : renderer(renderer) {}

        ResourceDeleter(const ResourceDeleter &) = default;

        ResourceDeleter(ResourceDeleter &&) = default;

        ResourceDeleter &operator=(const ResourceDeleter &) = default;

        ResourceDeleter &operator=(ResourceDeleter &&) = default;

        ~ResourceDeleter() = default;

        void operator()(Buffer &b) const { renderer->deleteBufferInternal(b); }
    };

    void recreateSwapchain();
    void recreateRingBuffer(unsigned int newSize);

    unsigned int ringBufferAllocate(unsigned int size, unsigned int alignPower);

    void deleteBufferInternal(Buffer &b);

public:
#pragma mark - Lifecycle

    explicit Renderer(const RendererInfo &rendererInfo);

    Renderer(const Renderer &) = delete;

    Renderer &operator=(const Renderer &) = delete;

    Renderer(Renderer &&other) = default;

    Renderer &operator=(Renderer &&other) = default;

    ~Renderer();

#pragma mark - Resource Management

    BufferHandle createBuffer(BufferType type, uint32_t size, const void *contents);

    void deleteBuffer(BufferHandle handle);
};

}; // namespace renderer
}; // namespace vkmol

#endif // VKMOL_RENDERER_RENDERER_H
