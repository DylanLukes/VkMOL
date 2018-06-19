#ifndef VKMOL_RENDERER_RESOURCE_H
#define VKMOL_RENDERER_RESOURCE_H

#include <cstdint>
#include <type_traits>

namespace vkmol {
namespace renderer {

template <class T>
class ResourceContainer;

template <class T>
class ResourceHandle {
private:
    friend class ResourceContainer<T>;

    uint32_t id;

    explicit ResourceHandle(uint32_t handle);

public:
    ResourceHandle();

    ~ResourceHandle() = default;

#pragma mark - Lifecycle

    ResourceHandle(const ResourceHandle<T> &other);
    ResourceHandle &operator=(const ResourceHandle<T> &other);

    ResourceHandle(ResourceHandle<T> &&other);
    ResourceHandle &operator=(ResourceHandle<T> &&other);

#pragma mark - Boolean Utilities

    bool operator==(const ResourceHandle<T> &other) const;

    explicit operator bool() const;
};

}; // namespace renderer
}; // namespace vkmol

#endif //VKMOL_RENDERER_RESOURCE_H
