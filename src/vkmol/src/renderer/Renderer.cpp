/*
  Copyright 2018, Dylan Lukes

  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to
  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
  of the Software, and to permit persons to whom the Software is furnished to do
  so, subject to the following conditions:

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

#include "vkmol/private/Utilities.h"
#include "vkmol/private/vma/vk_mem_alloc.h"
#include "vkmol/renderer/Renderer.h"

static PFN_vkCreateDebugReportCallbackEXT pfn_vkCreateDebugReportCallbackEXT =
    nullptr;
static PFN_vkDestroyDebugReportCallbackEXT pfn_vkDestroyDebugReportCallbackEXT =
    nullptr;
static PFN_vkDebugMarkerSetObjectNameEXT pfn_vkDebugMarkerSetObjectNameEXT =
    nullptr;

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

Renderer::Renderer(const RendererInfo &info) {
    bool enableValidation = info.debug;
    bool enableMarkers    = info.trace;
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

    return bufferHandle;
}

void Renderer::deleteBuffer(BufferHandle handle) {
    buffers.removeWith(handle, [this](Buffer &b) {
        this->graveyard.emplace(std::move(b));
    });
}

}; // namespace renderer
}; // namespace vkmol
