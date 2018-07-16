/*
  Copyright 2015-2018 Turo Lamminen, 2018 Dylan Lukes

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

#include "vkmol/private/Debug.h"
#include "vkmol/private/Utilities.h"
#include "vkmol/private/loguru/loguru.hpp"
#include "vkmol/private/vma/vk_mem_alloc.h"
#include "vkmol/renderer/Renderer.h"
#include <bitset>

namespace vkmol {
namespace renderer {

#pragma mark - Utilities

vk::BufferUsageFlags bufferTypeUsageFlags(BufferType type) {
    vk::BufferUsageFlags flags;
    switch (type) {
    case BufferType::Invalid: UNREACHABLE();
    case BufferType::Vertex: flags |= vk::BufferUsageFlagBits::eVertexBuffer; break;
    case BufferType::Index: flags |= vk::BufferUsageFlagBits::eIndexBuffer; break;
    case BufferType::Uniform: flags |= vk::BufferUsageFlagBits::eUniformBuffer; break;
    }

    return flags;
}

#pragma mark - Lifecycle

Renderer::Renderer(const RendererInfo &rendererInfo)
: isSwapchainDirty(true), currentFrame(0), lastSyncedFrame(0), uboAlignment(0), ssboAlignment(0) {
    auto [versionMajor, versionMinor, versionPatch] = rendererInfo.appVersion;

    LOG_SCOPE_F(INFO,
                "Initializing renderer for %s %d.%d.%d",
                rendererInfo.appName.data(),
                versionMajor,
                versionMinor,
                versionPatch);

    bool enableValidation = rendererInfo.debug;
    bool enableMarkers    = rendererInfo.trace;

    delegate = rendererInfo.delegate;

    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName   = rendererInfo.appName.data();
    appInfo.applicationVersion = VK_MAKE_VERSION(versionMajor, versionMinor, versionPatch);
    appInfo.pEngineName        = "VkMOL";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);

    vk::InstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    std::vector<const char *> extensions = delegate.getInstanceExtensions();

    std::vector<const char *> layers = {"VK_LAYER_LUNARG_standard_validation"};

    if (enableValidation) {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        instanceCreateInfo.enabledLayerCount   = layers.size();
        instanceCreateInfo.ppEnabledLayerNames = layers.data();
    }

    LOG_F(INFO, "Active instance extensions:");
    for (const auto &ext : extensions) { LOG_F(INFO, "\t- %s", ext); }

    instanceCreateInfo.enabledExtensionCount   = extensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    instance = vk::createInstance(instanceCreateInfo);

    if (enableValidation) {
        LOG_F(INFO, "Enabling validation layers...");
        pfn_vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
            instance.getProcAddr("vkCreateDebugReportCallbackEXT"));
        pfn_vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
            instance.getProcAddr("vkDestroyDebugReportCallbackEXT"));

        vk::DebugReportCallbackCreateInfoEXT callbackInfo;
        callbackInfo.flags       = vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning;
        callbackInfo.pfnCallback = debugCallbackFunc;
        debugCallback            = instance.createDebugReportCallbackEXT(callbackInfo);
    }

    if (enableMarkers) {
        LOG_F(INFO, "Enabling debug markers...");
        pfn_vkDebugMarkerSetObjectNameEXT =
            reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(instance.getProcAddr("vkDebugMarkerSetObjectNameEXT"));
    }

    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();

    if (physicalDevices.empty()) {
        LOG_F(ERROR, "No physical Vulkan devices available.");
        instance.destroy();
        instance = nullptr;
        throw std::runtime_error("No physical Vulkan devices available.");
    }

    LOG_F(INFO, "%zu physical devices available.", physicalDevices.size());
    physicalDevice = physicalDevices.at(0);

    deviceProperties = physicalDevice.getProperties();
    LOG_F(INFO, "Selecting device:");

    LOG_F(INFO,
          "\tDevice API version: %u.%u.%u",
          VK_VERSION_MAJOR(deviceProperties.apiVersion),
          VK_VERSION_MINOR(deviceProperties.apiVersion),
          VK_VERSION_PATCH(deviceProperties.apiVersion));

    LOG_F(INFO,
          "\tDriver version: %u.%u.%u (%u) (%#08x)",
          VK_VERSION_MAJOR(deviceProperties.driverVersion),
          VK_VERSION_MINOR(deviceProperties.driverVersion),
          VK_VERSION_PATCH(deviceProperties.driverVersion),
          deviceProperties.driverVersion,
          deviceProperties.driverVersion);

    LOG_F(INFO, "\tVendor ID: %x", deviceProperties.vendorID);
    LOG_F(INFO, "\tDevice ID: %x", deviceProperties.deviceID);
    LOG_F(INFO, "\tType: %s", vk::to_string(deviceProperties.deviceType).c_str());
    LOG_F(INFO, "\tName: \"%s\"", deviceProperties.deviceName);

    LOG_F(INFO, "\tLimits: ");
    LOG_F(INFO, "\t\tUniform buffer alignment: %llu", deviceProperties.limits.minUniformBufferOffsetAlignment);
    LOG_F(INFO, "\t\tStorage buffer alignment: %llu", deviceProperties.limits.minStorageBufferOffsetAlignment);
    LOG_F(INFO, "\t\tTexel buffer alignment:   %llu", deviceProperties.limits.minTexelBufferOffsetAlignment);

    uboAlignment  = deviceProperties.limits.minUniformBufferOffsetAlignment;
    ssboAlignment = deviceProperties.limits.minStorageBufferOffsetAlignment;

    deviceFeatures = physicalDevice.getFeatures();

    surface = delegate.getSurface(instance);

    memoryProperties = physicalDevice.getMemoryProperties();
    LOG_F(INFO, "Memory properties: ");

    LOG_F(INFO, "\tTypes:");
    for (unsigned int i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        LOG_F(INFO,
              "\t\t%u. Heap %u %s",
              i,
              memoryProperties.memoryTypes[i].heapIndex,
              vk::to_string(memoryProperties.memoryTypes[i].propertyFlags).c_str());
    }
    LOG_F(INFO, "\tHeaps:");
    for (unsigned int i = 0; i < memoryProperties.memoryHeapCount; i++) {
        std::string tempString = vk::to_string(memoryProperties.memoryHeaps[i].flags);
        LOG_F(INFO, "\t\t%u. Size ~%lluGiB %s", i, memoryProperties.memoryHeaps[i].size >> 30, tempString.c_str());
    }

    std::vector<vk::QueueFamilyProperties> queueProperties = physicalDevice.getQueueFamilyProperties();
    LOG_F(INFO, "Queue families: ");
    for (unsigned int i = 0; i < queueProperties.size(); ++i) {
        const auto &queue = queueProperties.at(i);

        LOG_F(INFO, "\t%u. Flags: %s", i, vk::to_string(queue.queueFlags).c_str());
        LOG_F(INFO, "\t   Count: %u", queue.queueCount);
        LOG_F(INFO, "\t   Timestamp valid bits: %u", queue.timestampValidBits);
        LOG_F(INFO,
              "\t   Image transfer granularity: (%u, %u, %u)",
              queue.minImageTransferGranularity.width,
              queue.minImageTransferGranularity.height,
              queue.minImageTransferGranularity.depth);

        if (queue.queueFlags & vk::QueueFlagBits::eGraphics) {
            if (physicalDevice.getSurfaceSupportKHR(i, surface)) {
                LOG_F(INFO, "\t   Can present to surface.");
                graphicsQueueIndex = i;
            } else {
                LOG_F(INFO, "\t   Cannot present to surface.");
            }
        }
    }

    if (graphicsQueueIndex == queueProperties.size()) {
        LOG_F(ERROR, "No suitable graphics queue.");
        throw std::runtime_error("No suitable graphics queue.");
    }

    LOG_F(INFO, "Using queue %u for graphics.", graphicsQueueIndex);

    std::array<float, 1>                     queuePriorities = {{0.0f}};
    std::array<vk::DeviceQueueCreateInfo, 2> queueCreateInfos;

    unsigned int queueCreateInfoCount = 0;

    queueCreateInfos[queueCreateInfoCount].queueFamilyIndex = graphicsQueueIndex;
    queueCreateInfos[queueCreateInfoCount].queueCount       = 1;
    queueCreateInfos[queueCreateInfoCount].pQueuePriorities = &queuePriorities[0];
    queueCreateInfoCount++;

    transferQueueIndex = graphicsQueueIndex;
    auto currentFlags  = std::bitset<32>(static_cast<uint32_t>(queueProperties[graphicsQueueIndex].queueFlags));
    for (unsigned int i = 0; i < queueProperties.size(); ++i) {
        if (i == graphicsQueueIndex) continue;

        const auto &queue = queueProperties.at(i);
        if (!(queue.queueFlags & vk::QueueFlagBits::eTransfer)) continue;

        // The best transfer queue is the one with the *least* other capabilities.
        auto queueFlags = std::bitset<32>(static_cast<uint32_t>(queue.queueFlags));
        if (currentFlags.count() < queueFlags.count()) {
            transferQueueIndex = i;
            currentFlags       = queueFlags;
        }
    }

    if (transferQueueIndex != graphicsQueueIndex) {
        LOG_F(INFO, "Using queue %u for transfer", transferQueueIndex);
        queueCreateInfos[queueCreateInfoCount].queueFamilyIndex = transferQueueIndex;
        queueCreateInfos[queueCreateInfoCount].queueCount       = 1;
        queueCreateInfos[queueCreateInfoCount].pQueuePriorities = &queuePriorities[0];
        queueCreateInfoCount++;
    } else {
        LOG_F(INFO, "Sharing queue %u for transfer.", transferQueueIndex);
    }

    std::unordered_set<std::string> availableExtensions;
    LOG_F(INFO, "Available device extensions:");
    for (const auto &ext : physicalDevice.enumerateDeviceExtensionProperties()) {
        LOG_F(INFO, "\t- %s v%d", ext.extensionName, ext.specVersion);
        availableExtensions.insert(ext.extensionName);
    }

    std::vector<const char *> deviceExtensions;

    auto checkExtension = [&deviceExtensions, &availableExtensions](const char *ext) {
        if (availableExtensions.find(std::string(ext)) != availableExtensions.end()) {
            LOG_F(INFO, "Activating optional device extension: %s", ext);
            deviceExtensions.push_back(ext);
            return true;
        }
        return false;
    };

    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    // This is an optimization for Vulkan Memory Allocator we use when available.
    // See: http://www.asawicki.info/articles/VK_KHR_dedicated_allocation.php5
    bool dedicatedAllocation = checkExtension(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME) &&
                               checkExtension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);

    if (enableMarkers) { debugMarkers = checkExtension(VK_EXT_DEBUG_MARKER_EXTENSION_NAME); }

    vk::DeviceCreateInfo deviceCreateInfo;
    assert(queueCreateInfoCount <= queueCreateInfos.size());
    deviceCreateInfo.queueCreateInfoCount    = queueCreateInfoCount;
    deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures        = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount   = deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    if (enableValidation) {
        deviceCreateInfo.enabledLayerCount   = layers.size();
        deviceCreateInfo.ppEnabledLayerNames = layers.data();
    }

    device = physicalDevice.createDevice(deviceCreateInfo);

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice         = physicalDevice;
    allocatorInfo.device                 = device;
    if (dedicatedAllocation) {
        LOG_F(INFO, "Dedicated allocations are enabled (with VMA).");
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
    }

    vmaCreateAllocator(&allocatorInfo, &allocator);

    graphicsQueue = device.getQueue(graphicsQueueIndex, 0);
    transferQueue = device.getQueue(transferQueueIndex, 0);

    auto surfacePresentModes_ = physicalDevice.getSurfacePresentModesKHR(surface);
    surfacePresentModes.reserve(surfacePresentModes_.size());

    LOG_F(INFO, "Surface present modes:");
    for (const auto &presentMode : surfacePresentModes_) {
        LOG_F(INFO, "\t- %s", vk::to_string(presentMode).c_str());
        surfacePresentModes.insert(presentMode);
    }

    LOG_F(INFO, "Surface formats:");
    auto surfaceFormats_ = physicalDevice.getSurfaceFormatsKHR(surface);
    for (const auto &surfaceFormat : surfaceFormats_) {
        LOG_F(INFO,
              "\t- %s %s",
              vk::to_string(surfaceFormat.format).c_str(),
              vk::to_string(surfaceFormat.colorSpace).c_str());

        // TODO: Should fallback to unorm888? sRGB is better.
        if (surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            surfaceFormats.insert(surfaceFormat.format);
        }
    }

    // Note: MSAA sampling would go here.

    recreateSwapchain();
    recreateRingBuffer(rendererInfo.ringBufferSize);

    acquireSemaphore = device.createSemaphore(vk::SemaphoreCreateInfo());
    finishedSemaphore = device.createSemaphore(vk::SemaphoreCreateInfo());

    vk::CommandPoolCreateInfo poolInfo;
    poolInfo.queueFamilyIndex = transferQueueIndex;
    transferCommandPool = device.createCommandPool(poolInfo);

// TODO: USE A PIPELINE CACHE!!! But this is trickier on mobile,
// probably requires a delegate function to inform it where to look.

//    vk::PipelineCacheCreateInfo cacheInfo;
//    std::vector<char> cacheData;
//    std::string plCacheFile =  spirvCacheDir + "pipeline.cache";
//    if (!desc.skipShaderCache && fileExists(plCacheFile)) {
//        cacheData                 = readFile(plCacheFile);
//        cacheInfo.initialDataSize = cacheData.size();
//        cacheInfo.pInitialData    = cacheData.data();
//    }
//
//    pipelineCache = device.createPipelineCache(cacheInfo);
}

void Renderer::deleteBufferInternal(Buffer &b) {
    assert(b.allocationType != BufferAllocationType::Default);
    assert(b.lastUsedFrame <= lastSyncedFrame);
    this->device.destroyBuffer(b.buffer);
    assert(b.memory != nullptr);
    vmaFreeMemory(this->allocator, b.memory);
    assert(b.type != BufferType::Invalid);

    b.buffer          = vk::Buffer();
    b.allocationType  = BufferAllocationType::Default;
    b.memory          = 0;
    b.size            = 0;
    b.offset          = 0;
    b.lastUsedFrame   = 0;
    b.type            = BufferType::Invalid;
}

void Renderer::recreateSwapchain() {

}

void Renderer::recreateRingBuffer(unsigned int newSize) {

}

Renderer::~Renderer() {

    // todo: could write out pipeline cache here (!)

    device.destroySemaphore(finishedSemaphore);
    finishedSemaphore = vk::Semaphore();

    device.destroySemaphore(acquireSemaphore);
    acquireSemaphore = vk::Semaphore();

    vmaFreeMemory(allocator, ringBufferMemory);
    ringBufferMemory = nullptr;
    persistentMapping = nullptr;
    device.destroyBuffer(ringBuffer);
    ringBuffer = vk::Buffer();

    // destroy swapchain

    instance.destroySurfaceKHR(surface);
    surface = vk::SurfaceKHR();

    vmaDestroyAllocator(allocator);
    allocator = VK_NULL_HANDLE;

    // destroy command pool

    if (debugCallback) {
        instance.destroyDebugReportCallbackEXT(debugCallback);
        debugCallback = vk::DebugReportCallbackEXT();
    }

    device.destroyCommandPool(transferCommandPool);
    transferCommandPool = vk::CommandPool();

    device.destroy();
    device = vk::Device();

    instance.destroy();
    instance = vk::Instance();
}

#pragma mark - Resource Management

BufferHandle Renderer::createBuffer(BufferType type, uint32_t size, const void *contents) {
    assert(type != BufferType::Invalid);
    assert(size != 0);
    assert(contents != nullptr);

    vk::BufferCreateInfo info;
    info.size  = size;
    info.usage = bufferTypeUsageFlags(type);

    auto [buffer, bufferHandle] = buffers.add();
    buffer.buffer               = device.createBuffer(info);

    VmaAllocationCreateInfo allocInfo;

    // todo ...

    return bufferHandle;
}

void Renderer::deleteBuffer(BufferHandle handle) {
    buffers.removeWith(std::move(handle), [this](Buffer &b) { this->graveyard.emplace(std::move(b)); });
}

}; // namespace renderer
}; // namespace vkmol
