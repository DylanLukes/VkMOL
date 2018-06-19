//
// Created by Dylan Lukes on 6/19/18.
//

#ifndef VKMOL_RENDERER_BUFFER_H
#define VKMOL_RENDERER_BUFFER_H

#include <vulkan/vulkan.hpp>

#include <climits>
#include <cstdint> // required by vk_mem_alloc.h

#include <vkmol/internal/vma/vk_mem_alloc.h>

namespace vkmol {
namespace renderer {

enum class BufferType : uint8_t {
    Invalid,
    Vertex,
    Index,
    Uniform
};

enum class BufferAllocationType : bool {
    Default = false,
    Ring    = true
};

struct Buffer {
    BufferType           type;
    BufferAllocationType allocationType;

    uint32_t size;
    uint32_t offset;

    vk::Buffer    buffer;
    VmaAllocation memory;

    uint32_t lastUsedFrame;

#pragma mark - Lifecycle

    Buffer();

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    Buffer(Buffer &&other);
    Buffer &operator=(Buffer &&);

    ~Buffer();

#pragma mark - Operations

    bool operator==(const Buffer &other) const;
    size_t getHash() const;
};

}; // namespace renderer
}; // namespace vkmol

#endif //VKMOL_BUFFER_H
