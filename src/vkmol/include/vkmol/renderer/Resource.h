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

#ifndef VKMOL_RENDERER_RESOURCE_H
#define VKMOL_RENDERER_RESOURCE_H

#include "Buffer.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace vkmol {
namespace renderer {

#pragma mark - Resource Handle

template <class T>
class ResourceContainer;

template <class T>
class ResourceHandle {
private:
    friend class ResourceContainer<T>;

    uint32_t id;

    explicit ResourceHandle(uint32_t handle) : id(handle){};

public:
    ResourceHandle() : id(0){};

    ~ResourceHandle() = default;

#pragma mark - Lifecycle

    ResourceHandle(const ResourceHandle<T> &other) : id(other.id) {}
    ResourceHandle &operator=(const ResourceHandle<T> &other) {
        id = other.id;
        return *this;
    }

    ResourceHandle(ResourceHandle<T> &&other) : id(other.id) {}
    ResourceHandle &operator=(ResourceHandle<T> &&other) {
        id       = other.id;
        other.id = 0;
        return *this;
    }

#pragma mark - Boolean Utilities

    bool operator==(const ResourceHandle<T> &other) const {
        return id == other.id;
    }

    explicit operator bool() const { return id != 0; }
};

#pragma mark - Resource Container

template <class T>
class ResourceContainer {
private:
    std::unordered_map<uint32_t, T> resources;
    unsigned int                    next;

public:
#pragma mark - Lifecycle

    ResourceContainer() : next(1){};

    ResourceContainer(const ResourceContainer<T> &) = delete;
    ResourceContainer &operator=(const ResourceContainer<T> &) = delete;

    ResourceContainer(ResourceContainer<T> &&) = delete;
    ResourceContainer &operator=(ResourceContainer<T> &&) = delete;

    ~ResourceContainer() = default;

#pragma mark - Operations

    std::pair<T &, ResourceHandle<T>> add() {
        unsigned int handle = next++;

        auto result = resources.emplace(handle, T());
        assert(result.second);

        return std::make_pair(std::ref(result.first->second),
                              ResourceHandle<T>(handle));
    }

    const T &get(ResourceHandle<T> handle) const {
        assert(handle.id != 0);

        auto it = resources.find(handle.id);
        assert(it != std::end(resources));

        return it->second;
    }

    T &&get(ResourceHandle<T> handle) {
        assert(handle.id != 0);

        auto it = resources.find(handle.id);
        assert(it != std::end(resources));

        return it->second;
    }

    void remove(ResourceHandle<T> handle) {
        assert(handle.id != 0);

        auto it = resources.find(handle.id);
        assert(it != std::end(resources));

        resources.erase(it);
    }

    template <typename F>
    void removeWith(ResourceHandle<T> handle, F &&f) {
        assert(handle.id != 0);

        auto it = resources.find(handle.id);
        assert(it != std::end(resources));

        f(it->second);
        resources.erase(it);
    }

    template <typename F>
    void clearWith(F &&f) {
        auto it = std::begin(resources);
        while (it != std::end(resources)) {
            f(it->second);
            it = resources.erase(it);
        }
    }
};

#pragma mark - Variant Declaration

typedef std::variant<Buffer> Resource;

#pragma mark - Resource Visitors

struct ResourceHasher final {
    size_t operator()(const Buffer &b) const;
};

}; // namespace renderer
}; // namespace vkmol

namespace std {

template <>
struct hash<vkmol::renderer::Resource> {
    size_t operator()(const vkmol::renderer::Resource &r) const {
        return std::visit(vkmol::renderer::ResourceHasher(), r);
    }
};
} // namespace std

#endif // VKMOL_RENDERER_RESOURCE_H
