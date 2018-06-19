#ifndef VKMOL_RENDERER_RESOURCECONTAINER_H
#define VKMOL_RENDERER_RESOURCECONTAINER_H

#include <unordered_map>

namespace vkmol {
namespace renderer {

template <class T>
class ResourceHandle;

template <class T>
class ResourceContainer {
private:
    std::unordered_map<uint32_t, T> resources;
    unsigned int                    next;

public:
#pragma mark - Lifecycle

    ResourceContainer();

    ResourceContainer(const ResourceContainer<T> &) = delete;
    ResourceContainer &operator=(const ResourceContainer<T> &) = delete;

    ResourceContainer(ResourceContainer<T> &&) = delete;
    ResourceContainer &operator=(ResourceContainer<T> &&) = delete;

    ~ResourceContainer() = default;

#pragma mark - Operations

    std::pair<T &, ResourceHandle<T>> add();

    const T &get(ResourceHandle<T> handle) const;

    T &&get(ResourceHandle<T> handle);

    void remove(ResourceHandle<T> handle);

    template <typename F>
    void removeWith(ResourceHandle<T> handle, F &&f);

    template <typename F>
    void clearWith(F &&f);
};

}; // namespace renderer
}; // namespace vkmol

#endif // VKMOL_RENDERER_RESOURCECONTAINER_H
