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

#include "vkmol/renderer/Buffer.h"

namespace vkmol {
namespace renderer {

Buffer::Buffer()
: type(BufferType::Invalid)
, allocationType(BufferAllocationType::Default)
, size(0)
, offset(0)
, memory(nullptr)
, lastUsedFrame(0) {}

Buffer::Buffer(Buffer &&other)
: type(other.type)
, allocationType(other.allocationType)
, size(other.size)
, offset(other.offset)
, buffer(other.buffer)
, memory(other.memory)
, lastUsedFrame(other.lastUsedFrame) {
    other.type           = BufferType::Invalid;
    other.allocationType = BufferAllocationType::Default;
    other.size           = 0;
    other.offset         = 0;
    other.buffer         = vk::Buffer();
    other.memory         = nullptr;
    other.lastUsedFrame  = 0;
}

Buffer &Buffer::operator=(Buffer &&other) {
    if (this == &other) { return *this; }

    assert(!buffer);
    assert(!memory);

    type           = other.type;
    allocationType = other.allocationType;
    size           = other.size;
    offset         = other.offset;
    buffer         = other.buffer;
    memory         = other.memory;
    lastUsedFrame  = other.lastUsedFrame;

    other.type           = BufferType::Invalid;
    other.allocationType = BufferAllocationType::Default;
    other.size           = 0;
    other.offset         = 0;
    other.buffer         = vk::Buffer();
    other.memory         = nullptr;
    other.lastUsedFrame  = 0;

    // Should not be move assigning valid buffers.
    assert(type == BufferType::Invalid);

    return *this;
}

Buffer::~Buffer() {
    assert(allocationType != BufferAllocationType::Ring);
    assert(size == 0);
    assert(offset == 0);
    assert(!buffer);
    assert(!memory);
}

bool Buffer::operator==(const Buffer &other) const {
    // todo: is this sufficient?
    return this->buffer == other.buffer;
}

size_t Buffer::getHash() const {
    return std::hash<uint64_t>()(reinterpret_cast<uint64_t>(VkBuffer(buffer)));
}

}; // namespace renderer
}; // namespace vkmol