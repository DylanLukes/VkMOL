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

#include <vkmol/renderer/UploadOp.h>

#include "vkmol/renderer/UploadOp.h"

namespace vkmol {
namespace renderer {

UploadOp::UploadOp(UploadOp &&other) noexcept
: commandBuffer(other.commandBuffer)
, semaphore(other.semaphore)
, semaphoreWaitMask(other.semaphoreWaitMask)
, stagingBuffer(other.stagingBuffer)
, memory(other.memory)
, allocationInfo(other.allocationInfo)
, imageAcquireBarriers(std::move(other.imageAcquireBarriers))
, bufferAcquireBarriers(std::move(other.bufferAcquireBarriers)) {
    other.commandBuffer     = vk::CommandBuffer();
    other.semaphore         = vk::Semaphore();
    other.semaphoreWaitMask = vk::PipelineStageFlags();
    other.stagingBuffer     = vk::Buffer();
    other.memory            = nullptr;
    assert(other.imageAcquireBarriers.empty());
    assert(other.bufferAcquireBarriers.empty());
}

UploadOp &UploadOp::operator=(UploadOp &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    assert(!commandBuffer);
    assert(!semaphore);
    assert(!semaphoreWaitMask);
    assert(!stagingBuffer);
    assert(!memory);
    assert(imageAcquireBarriers.empty());
    assert(bufferAcquireBarriers.empty());

    commandBuffer = other.commandBuffer;
    other.commandBuffer     = vk::CommandBuffer();

    semaphore = other.semaphore;
    other.semaphore         = vk::Semaphore();

    semaphoreWaitMask = other.semaphoreWaitMask;
    other.semaphoreWaitMask = vk::PipelineStageFlags();

    stagingBuffer = other.stagingBuffer;
    other.stagingBuffer     = vk::Buffer();

    memory = other.memory;
    other.memory            = nullptr;

    imageAcquireBarriers     = std::move(other.imageAcquireBarriers);
    assert(other.imageAcquireBarriers.empty());

    bufferAcquireBarriers     = std::move(other.bufferAcquireBarriers);
    assert(other.bufferAcquireBarriers.empty());

    allocationInfo      = other.allocationInfo;

    return *this;
}

UploadOp::~UploadOp() noexcept {
    assert(!commandBuffer);
    assert(!semaphore);
    assert(!semaphoreWaitMask);
    assert(!stagingBuffer);
    assert(!memory);
}

}; // namespace renderer
}; // namespace vkmol