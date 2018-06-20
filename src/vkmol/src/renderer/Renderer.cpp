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

#include <vkmol/renderer/Renderer.h>

#include "vkmol/private/Debug.h"
#include "vkmol/private/Utilities.h"
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

Renderer::Renderer(const RendererInfo &rendererInfo) {
    bool enableValidation = rendererInfo.debug;
    bool enableMarkers    = rendererInfo.trace;

    delegate = rendererInfo.delegate;

    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName   = rendererInfo.appName.data();
    appInfo.applicationVersion = rendererInfo.appVersion;
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

    instanceCreateInfo.enabledExtensionCount   = extensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    instance = vk::createInstance(instanceCreateInfo);

    if (enableValidation) {
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
        pfn_vkDebugMarkerSetObjectNameEXT =
            reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(
                instance.getProcAddr("vkDebugMarkerSetObjectNameEXT"));
    }

    std::vector<vk::PhysicalDevice> physicalDevices =
        instance.enumeratePhysicalDevices();


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
