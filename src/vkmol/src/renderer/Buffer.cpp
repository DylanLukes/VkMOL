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
, buffer(other.buffer)
, offset(other.offset)
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