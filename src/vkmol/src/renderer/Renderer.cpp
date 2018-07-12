/*
  Copyright 2018, Dylan Lukes

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

namespace vkmol {
namespace renderer {

#pragma mark - Utilities

vk::BufferUsageFlags bufferTypeUsageFlags(BufferType type) {
    vk::BufferUsageFlags flags;
    switch (type) {
    case BufferType::Invalid: UNREACHABLE();
    case BufferType::Vertex:
        flags |= vk::BufferUsageFlagBits::eVertexBuffer;
        break;
    case BufferType::Index:
        flags |= vk::BufferUsageFlagBits::eIndexBuffer;
        break;
    case BufferType::Uniform:
        flags |= vk::BufferUsageFlagBits::eUniformBuffer;
        break;
    }

    return flags;
}

#pragma mark - Lifecycle

Renderer::Renderer(const RendererInfo &rendererInfo)
: isSwapchainDirty(true), uboAlignment(0), ssboAlignment(0) {
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
    appInfo.pApplicationName = rendererInfo.appName.data();
    appInfo.applicationVersion =
        VK_MAKE_VERSION(versionMajor, versionMinor, versionPatch);
    appInfo.pEngineName   = "VkMOL";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    vk::InstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    std::vector<const char *> extensions = delegate.getInstanceExtensions();

    std::vector<const char *> layers = {"VK_LAYER_LUNARG_standard_validation"};

    if (enableValidation) {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        instanceCreateInfo.enabledLayerCount   = layers.size();
        instanceCreateInfo.ppEnabledLayerNames = layers.data();
    }

    {
        LOG_F(INFO, "Active instance extensions:");
        for (const auto &ext : extensions) { LOG_F(INFO, "\t%s", ext); }
    }

    instanceCreateInfo.enabledExtensionCount   = extensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    instance = vk::createInstance(instanceCreateInfo);

    if (enableValidation) {
        LOG_F(INFO, "Enabling validation layers...");
        pfn_vkCreateDebugReportCallbackEXT =
            reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
                instance.getProcAddr("vkCreateDebugReportCallbackEXT"));
        pfn_vkDestroyDebugReportCallbackEXT =
            reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
                instance.getProcAddr("vkDestroyDebugReportCallbackEXT"));

        vk::DebugReportCallbackCreateInfoEXT callbackInfo;
        callbackInfo.flags = vk::DebugReportFlagBitsEXT::eError |
                             vk::DebugReportFlagBitsEXT::eWarning;
        callbackInfo.pfnCallback = debugCallbackFunc;
        debugCallback = instance.createDebugReportCallbackEXT(callbackInfo);
    }

    if (enableMarkers) {
        LOG_F(INFO, "Enabling debug markers...");
        pfn_vkDebugMarkerSetObjectNameEXT =
            reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(
                instance.getProcAddr("vkDebugMarkerSetObjectNameEXT"));
    }

    std::vector<vk::PhysicalDevice> physicalDevices =
        instance.enumeratePhysicalDevices();

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
    LOG_F(
        INFO, "\tType: %s", vk::to_string(deviceProperties.deviceType).c_str());
    LOG_F(INFO, "\tName: \"%s\"", deviceProperties.deviceName);

    LOG_F(INFO, "\tLimits: ");
    LOG_F(INFO,
          "\t\tUniform buffer alignment: %llu",
          deviceProperties.limits.minUniformBufferOffsetAlignment);
    LOG_F(INFO,
          "\t\tStorage buffer alignment: %llu",
          deviceProperties.limits.minStorageBufferOffsetAlignment);
    LOG_F(INFO,
          "\t\tTexel buffer alignment:   %llu",
          deviceProperties.limits.minTexelBufferOffsetAlignment);

    uboAlignment  = deviceProperties.limits.minUniformBufferOffsetAlignment;
    ssboAlignment = deviceProperties.limits.minStorageBufferOffsetAlignment;

    surface = delegate.getSurface(instance);

    memoryProperties = physicalDevice.getMemoryProperties();
    LOG_F(INFO, "Memory properties: ");

    LOG_F(INFO, "\tTypes:");
    for (unsigned int i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        LOG_F(INFO,
              "\t\t%u. Heap %u %s",
              i,
              memoryProperties.memoryTypes[i].heapIndex,
              vk::to_string(memoryProperties.memoryTypes[i].propertyFlags)
                  .c_str());
    }
    LOG_F(INFO, "\tHeaps:");
    for (unsigned int i = 0; i < memoryProperties.memoryHeapCount; i++) {
        std::string tempString =
            vk::to_string(memoryProperties.memoryHeaps[i].flags);
        LOG_F(INFO,
              "\t\t%u. Size ~%lluGiB %s",
              i,
              memoryProperties.memoryHeaps[i].size >> 30,
              tempString.c_str());
    }


}

Renderer::~Renderer() {
    if (debugCallback) {
        instance.destroyDebugReportCallbackEXT(debugCallback);
        debugCallback = vk::DebugReportCallbackEXT();
    }

    instance.destroy();
    instance = vk::Instance();
}

#pragma mark - Resource Management

BufferHandle
Renderer::createBuffer(BufferType type, uint32_t size, const void *contents) {
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
    buffers.removeWith(std::move(handle), [this](Buffer &b) {
        this->graveyard.emplace(std::move(b));
    });
}

}; // namespace renderer
}; // namespace vkmol
