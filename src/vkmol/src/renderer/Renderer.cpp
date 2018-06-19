#include "vkmol/renderer/Renderer.h"
#include <vkmol/internal/Utilities.h>

#include <cassert>

namespace vkmol {
namespace renderer {

#pragma mark - Utilities

vk::BufferUsageFlags bufferTypeUsageFlags(BufferType type) {
    vk::BufferUsageFlags flags;
    switch (type) {
        case BufferType::Invalid:
            UNREACHABLE();
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
    // TODO: instantiate
}

#pragma mark - Resource Management

BufferHandle Renderer::createBuffer(BufferType type, uint32_t size, const void *contents) {
    assert(type != BufferType::Invalid);
    assert(size != 0);
    assert(contents != nullptr);

    vk::BufferCreateInfo info;
    info.size = size;
    info.usage = bufferTypeUsageFlags(type);


}

}; // namespace renderer
}; // namespace vkmol
