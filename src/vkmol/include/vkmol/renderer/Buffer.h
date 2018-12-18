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

#ifndef VKMOL_RENDERER_BUFFER_H
#define VKMOL_RENDERER_BUFFER_H

#include <vulkan/vulkan.hpp>

#include <climits>
#include <cstdint> // required by vk_mem_alloc.h

#include <vkmol/private/vma/vk_mem_alloc.h>

namespace vkmol {
namespace renderer {

enum class BufferType : uint8_t { Invalid, Vertex, Index, Uniform, Any };

enum class BufferAllocationType : bool { Default = false, Ring = true };

struct Buffer {
    BufferType           type;
    BufferAllocationType allocationType;
    uint32_t             size;
    uint32_t             offset;
    vk::Buffer           buffer;
    VmaAllocation        memory;

    uint32_t lastUsedFrame;

#pragma mark - Lifecycle

    Buffer();

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    Buffer(Buffer &&other) noexcept;
    Buffer &operator=(Buffer &&) noexcept;

    ~Buffer();

#pragma mark - Operations

    bool   operator==(const Buffer &other) const;
    size_t getHash() const;
};

}; // namespace renderer
}; // namespace vkmol

#endif // VKMOL_BUFFER_H
